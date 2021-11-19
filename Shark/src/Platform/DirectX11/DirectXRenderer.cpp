#include "skpch.h"
#include "DirectXRenderer.h"
#include "Shark/Core/Application.h"

#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"
#include "Platform/DirectX11/DirectXFrameBuffer.h"
#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"
#include "Platform/DirectX11/DirectXBuffers.h"
#include "Platform/DirectX11/DirectXPipeline.h"

#include "Shark/Debug/Instrumentor.h"
#include "Shark/Debug/Profiler.h"
#include "Shark/Core/Timer.h"

#include <backends/imgui_impl_dx11.h>


#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR(SK_STRINGIFY(call) "0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	static D3D11_PRIMITIVE_TOPOLOGY SharkPrimitveTopologyToD3D11(PrimitveTopology topology)
	{
		switch (topology)
		{
			case PrimitveTopology::Triangle:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case PrimitveTopology::Line:      return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			case PrimitveTopology::Dot:       return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		}

		SK_CORE_ASSERT(false, "Unkonw Topology");
		return (D3D11_PRIMITIVE_TOPOLOGY)0;
	}

	DirectXRenderer* DirectXRenderer::s_Instance = nullptr;

	namespace Utils {

		static void LogAdapter(IDXGIAdapter* adapter)
		{
			DXGI_ADAPTER_DESC ad;
			SK_CHECK(adapter->GetDesc(&ad));
			char gpudesc[128];
			wcstombs_s(nullptr, gpudesc, ad.Description, 128);
			SK_CORE_INFO("GPU: {0}", gpudesc);
		}

	}

	DirectXRenderer::DirectXRenderer()
	{
		SK_PROFILE_FUNCTION();

		Init();
	}

	DirectXRenderer::~DirectXRenderer()
	{
		SK_PROFILE_FUNCTION();

		ShutDown();
	}

	void DirectXRenderer::Init()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(s_Instance == nullptr);
		s_Instance = this;

		SK_CHECK(CreateDXGIFactory(IID_PPV_ARGS(&m_Factory)));

		IDXGIAdapter* adapter = nullptr;
		SK_CHECK(m_Factory->EnumAdapters(0, &adapter));
		Utils::LogAdapter(adapter);

		UINT createdeviceFalgs = 0u;
#if SK_DEBUG
		createdeviceFalgs |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		SK_CHECK(D3D11CreateDevice(
			adapter,
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			createdeviceFalgs,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&m_Device,
			nullptr,
			&m_ImmediateContext
		));

		adapter->Release();

		auto& window = Application::Get().GetWindow();

		SwapChainSpecifications specs;
		specs.Widht = window.GetWidth();
		specs.Height = window.GetHeight();
		specs.WindowHandle = window.GetHandle();
		m_SwapChain = Ref<DirectXSwapChain>::Create(specs);


		m_ShaderLib = Ref<ShaderLibrary>::Create();

		m_ShaderLib->Load("assets/Shaders/Renderer2D_Quad.hlsl");
		m_ShaderLib->Load("assets/Shaders/Renderer2D_Circle.hlsl");
		m_ShaderLib->Load("assets/Shaders/Renderer2D_Line.hlsl");

		m_ShaderLib->Load("assets/Shaders/FullScreen.hlsl");
		m_ShaderLib->Load("assets/Shaders/CompositWidthDepth.hlsl");
		m_ShaderLib->Load("assets/Shaders/NegativeEffect.hlsl");
		m_ShaderLib->Load("assets/Shaders/BlurEffect.hlsl");

		uint32_t color = 0xFFFFFFFF;
		m_WhiteTexture = Texture2D::Create(1, 1, &color);

		VertexLayout layout = {
			{ VertexDataType::Float2, "Position" }
		};

		float vertices[4 * 2] = {
			-1.0f,  1.0f,
			 1.0f,  1.0f,
			 1.0f, -1.0f,
			-1.0f, -1.0f
		};

		uint32_t indices[3 * 2] = {
			0, 1, 2,
			2, 3, 0
		};

		m_QuadVertexBuffer = Ref<DirectXVertexBuffer>::Create(layout, vertices, sizeof(vertices));
		m_QuadIndexBuffer = Ref<DirectXIndexBuffer>::Create(indices, sizeof(indices) / sizeof(*indices));

		D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		m_Device->CreateSamplerState(&samplerDesc, &m_ClampSampler);

		// Frequency Query
		D3D11_QUERY_DESC queryDesc;
		queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		queryDesc.MiscFlags = 0;
		SK_CHECK(m_Device->CreateQuery(&queryDesc, &m_FrequencyQuery));

		m_PresentTimer = Ref<DirectXGPUTimer>::Create("Present");

	}

	void DirectXRenderer::ShutDown()
	{
		SK_PROFILE_FUNCTION();

		m_SwapChain = nullptr;
		m_ShaderLib = nullptr;
		m_WhiteTexture = nullptr;
		m_QuadVertexBuffer = nullptr;
		m_QuadIndexBuffer = nullptr;

		m_CommandBuffers.clear();

		if (m_ClampSampler)
		{
			m_ClampSampler->Release();
			m_ClampSampler = nullptr;
		}

		if (m_FrequencyQuery)
		{
			m_FrequencyQuery->Release();
			m_FrequencyQuery = nullptr;
		}

		if (m_Factory)
		{
			m_Factory->Release();
			m_Factory = nullptr;
		}

		if (m_ImmediateContext)
		{
			m_ImmediateContext->Release();
			m_ImmediateContext = nullptr;
		}

		if (m_Device)
		{
			m_Device->Release();
			m_Device = nullptr;
		}

		s_Instance = nullptr;
	}

	void DirectXRenderer::NewFrame()
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("DirectXRenderer::NewFrame");

		if (m_IsFirstFrame)
		{
			m_ImmediateContext->Begin(m_FrequencyQuery);
			m_FrequencyQueryActive = true;

			m_IsFirstFrame = false;
			return;
		}


		// SwapChain Resize

		if (m_SwapChainNeedsResize)
		{
			Flush();
			m_SwapChain->Resize(m_WindowWidth, m_WindowHeight);
			m_SwapChainNeedsResize = false;
		}


		// GPU Frequncy Query

		if (m_FrequencyQueryActive)
		{
			m_ImmediateContext->End(m_FrequencyQuery);
			m_FrequencyQueryActive = false;
		}

		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData;
		HRESULT hr = m_ImmediateContext->GetData(m_FrequencyQuery, &disjointData, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), D3D11_ASYNC_GETDATA_DONOTFLUSH);
		if (hr == S_OK)
		{
			m_GPUFrequency = disjointData.Frequency;
			m_IsValidFrequency = !disjointData.Disjoint;

			m_ImmediateContext->Begin(m_FrequencyQuery);
			m_FrequencyQueryActive = true;
		}

	}

	void DirectXRenderer::RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Image2D> image)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("DirectXRenderer::RenderFullScreenQuad");

		Ref<DirectXRenderCommandBuffer> dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>();
		ID3D11DeviceContext* ctx = dxCommandBuffer->GetContext();

		Ref<DirectXPipeline> dxPipeline = pipeline.As<DirectXPipeline>();

		Ref<DirectXShader> dxShader = dxPipeline->m_Shader;
		ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
		ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

		const UINT offset = 0;
		const UINT stride = m_QuadVertexBuffer->m_Layout.GetVertexSize();
		ctx->IASetVertexBuffers(0, 1, &m_QuadVertexBuffer->m_VertexBuffer, &stride, &offset);
		ctx->IASetIndexBuffer(m_QuadIndexBuffer->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		ctx->IASetInputLayout(dxShader->m_InputLayout);

		ctx->RSSetState(dxPipeline->m_RasterizerState);
		ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);

		Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;
		ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
		ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);
		ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

		Ref<DirectXImage2D> dxImage = image.As<DirectXImage2D>();
		ctx->PSSetShaderResources(0, 1, &dxImage->m_View);
		ctx->PSSetSamplers(0, 1, &m_ClampSampler);

		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx->DrawIndexed(6, 0, 0);
	}

	void DirectXRenderer::RenderFullScreenQuadWidthDepth(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Image2D> image, Ref<Image2D> depthImage)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("DirectXRenderer::RenderFullScreenQuadWidthDepth");

		Ref<DirectXRenderCommandBuffer> dxCommandBuffer = commandBuffer.As<DirectXRenderCommandBuffer>();
		ID3D11DeviceContext* ctx = dxCommandBuffer->GetContext();

		Ref<DirectXPipeline> dxPipeline = pipeline.As<DirectXPipeline>();

		Ref<DirectXShader> dxShader = dxPipeline->m_Shader;
		ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
		ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

		const UINT offset = 0;
		const UINT stride = m_QuadVertexBuffer->m_Layout.GetVertexSize();
		ctx->IASetVertexBuffers(0, 1, &m_QuadVertexBuffer->m_VertexBuffer, &stride, &offset);
		ctx->IASetIndexBuffer(m_QuadIndexBuffer->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		ctx->IASetInputLayout(dxShader->m_InputLayout);

		ctx->RSSetState(dxPipeline->m_RasterizerState);
		ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);

		Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;
		ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
		ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);
		ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

		Ref<DirectXImage2D> dxImage = image.As<DirectXImage2D>();
		ctx->PSSetShaderResources(0, 1, &dxImage->m_View);
		ctx->PSSetSamplers(0, 1, &m_ClampSampler);

		Ref<DirectXImage2D> dxDepthImage = depthImage.As<DirectXImage2D>();
		ctx->PSSetShaderResources(1, 1, &dxDepthImage->m_View);

		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx->DrawIndexed(6, 0, 0);
	}

	void DirectXRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<FrameBuffer> frameBuffer, Ref<Shader> shader, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_ASSERT(false, "not implementes");
#if 0
		Ref<DirectXRenderCommandBuffer> dxCommandBuffer = renderCommandBuffer.As<DirectXRenderCommandBuffer>();
		Ref<DirectXFrameBuffer> dxFrameBuffer = frameBuffer.As<DirectXFrameBuffer>();
		Ref<DirectXShader> dxShader = shader.As<DirectXShader>();
		Ref<DirectXConstantBufferSet> dxConstantBufferSet = constantBufferSet.As<DirectXConstantBufferSet>();
		Ref<DirectXTexture2DArray> dxTextureArray = textureArray.As<DirectXTexture2DArray>();
		Ref<DirectXVertexBuffer> dxVertexBuffer = vertexBuffer.As<DirectXVertexBuffer>();
		Ref<DirectXIndexBuffer> dxIndexBuffer = indexBuffer.As<DirectXIndexBuffer>();

		ID3D11DeviceContext* ctx = dxCommandBuffer->GetContext();
		

		ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
		ctx->OMSetDepthStencilState(dxFrameBuffer->m_DepthStencilState, 0);
		ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);

		ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
		ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

		for (auto&& [slot, cb] : dxConstantBufferSet->m_CBMap)
			ctx->VSSetConstantBuffers(slot, 1, &cb->m_ConstBuffer);

		if (dxTextureArray)
		{
			uint32_t slot = 0;
			for (auto t : dxTextureArray->m_TextureArray)
			{
				if (t)
				{
					ctx->PSSetShaderResources(slot, 1, &t->m_Image->m_View);
					ctx->PSSetSamplers(slot, 1, &t->m_Sampler);
				}
				slot++;
			}
		}

		const UINT offset = 0;
		const UINT stride = dxVertexBuffer->m_Layout.GetVertexSize();
		ctx->IASetVertexBuffers(0, 1, &dxVertexBuffer->m_VertexBuffer, &stride, &offset);
		ctx->IASetIndexBuffer(dxIndexBuffer->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		ctx->IASetInputLayout(dxShader->m_InputLayout);

		ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);
		ctx->IASetPrimitiveTopology(SharkPrimitveTopologyToD3D11(topology));

		ctx->DrawIndexed(indexCount, 0, 0);
#endif
	}

	void DirectXRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("DirectXRenderer::RenderGeometry [Indexed]");

		Ref<DirectXRenderCommandBuffer> commandBuffer = renderCommandBuffer.As<DirectXRenderCommandBuffer>();
		ID3D11DeviceContext* ctx = commandBuffer->GetContext();


		Ref<DirectXVertexBuffer> dxVB = vertexBuffer.As<DirectXVertexBuffer>();
		const UINT offset = 0;
		const UINT stride = dxVB->m_Layout.GetVertexSize();
		ctx->IASetVertexBuffers(0, 1, &dxVB->m_VertexBuffer, &stride, &offset);

		Ref<DirectXIndexBuffer> dxIB = indexBuffer.As<DirectXIndexBuffer>();
		ctx->IASetIndexBuffer(dxIB->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);


		Ref<DirectXPipeline> dxPipeline = pipeline.As<DirectXPipeline>();
		Ref<DirectXShader> dxShader = dxPipeline->m_Shader;

		ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
		ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

		// TODO(moro): move InputLayout to Pipeline
		ctx->IASetInputLayout(dxShader->m_InputLayout);
		
		Ref<DirectXConstantBufferSet> dxConstantBufferSet = constantBufferSet.As<DirectXConstantBufferSet>();
		for (auto&& [slot, cb] : dxConstantBufferSet->m_CBMap)
			ctx->VSSetConstantBuffers(slot, 1, &cb->m_ConstBuffer);

		Ref<DirectXTexture2DArray> dxTextureArray = textureArray.As<DirectXTexture2DArray>();
		if (dxTextureArray)
		{
			ctx->PSSetShaderResources(dxTextureArray->m_StartOffset, dxTextureArray->m_Count, dxTextureArray->m_Views.data());
			ctx->PSSetSamplers(dxTextureArray->m_StartOffset, dxTextureArray->m_Count, dxTextureArray->m_Samplers.data());
		}

		Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;

		ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
		ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

		ctx->RSSetState(dxPipeline->m_RasterizerState);
		ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);
		ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);

		ctx->IASetPrimitiveTopology(SharkPrimitveTopologyToD3D11(topology));

		ctx->DrawIndexed(indexCount, 0, 0);
	}

	void DirectXRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount, PrimitveTopology topology)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("DirectXRenderer::RenderGeometry");

		Ref<DirectXRenderCommandBuffer> commandBuffer = renderCommandBuffer.As<DirectXRenderCommandBuffer>();
		ID3D11DeviceContext* ctx = commandBuffer->GetContext();


		Ref<DirectXVertexBuffer> dxVB = vertexBuffer.As<DirectXVertexBuffer>();
		const UINT offset = 0;
		const UINT stride = dxVB->m_Layout.GetVertexSize();
		ctx->IASetVertexBuffers(0, 1, &dxVB->m_VertexBuffer, &stride, &offset);


		Ref<DirectXPipeline> dxPipeline = pipeline.As<DirectXPipeline>();
		Ref<DirectXShader> dxShader = dxPipeline->m_Shader;

		ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
		ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

		// TODO(moro): move InputLayout to Pipeline
		ctx->IASetInputLayout(dxShader->m_InputLayout);

		Ref<DirectXConstantBufferSet> dxConstantBufferSet = constantBufferSet.As<DirectXConstantBufferSet>();
		for (auto&& [slot, cb] : dxConstantBufferSet->m_CBMap)
			ctx->VSSetConstantBuffers(slot, 1, &cb->m_ConstBuffer);

		Ref<DirectXTexture2DArray> dxTextureArray = textureArray.As<DirectXTexture2DArray>();
		if (dxTextureArray)
		{
			ctx->PSSetShaderResources(dxTextureArray->m_StartOffset, dxTextureArray->m_Count, dxTextureArray->m_Views.data());
			ctx->PSSetSamplers(dxTextureArray->m_StartOffset, dxTextureArray->m_Count, dxTextureArray->m_Samplers.data());
		}

		Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;

		ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
		ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

		ctx->RSSetState(dxPipeline->m_RasterizerState);
		ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);
		ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);

		ctx->IASetPrimitiveTopology(SharkPrimitveTopologyToD3D11(topology));

		ctx->Draw(vertexCount, 0);
	}

	void DirectXRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("DirectXRenderer::RenderGeometry [Material]");

		Ref<DirectXRenderCommandBuffer> commandBuffer = renderCommandBuffer.As<DirectXRenderCommandBuffer>();
		ID3D11DeviceContext* ctx = commandBuffer->GetContext();


		Ref<DirectXVertexBuffer> dxVB = vertexBuffer.As<DirectXVertexBuffer>();
		const UINT offset = 0;
		const UINT stride = dxVB->m_Layout.GetVertexSize();
		ctx->IASetVertexBuffers(0, 1, &dxVB->m_VertexBuffer, &stride, &offset);

		Ref<DirectXIndexBuffer> dxIB = indexBuffer.As<DirectXIndexBuffer>();
		ctx->IASetIndexBuffer(dxIB->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);


		Ref<DirectXPipeline> dxPipeline = pipeline.As<DirectXPipeline>();
		Ref<DirectXShader> dxShader = dxPipeline->m_Shader;

		ctx->VSSetShader(dxShader->m_VertexShader, nullptr, 0);
		ctx->PSSetShader(dxShader->m_PixelShader, nullptr, 0);

		// TODO(moro): move InputLayout to Pipeline
		ctx->IASetInputLayout(dxShader->m_InputLayout);

		Ref<DirectXMaterial> dxMaterial = material.As<DirectXMaterial>();
		Ref<DirectXConstantBufferSet> dxCBSet = constantBufferSet.As<DirectXConstantBufferSet>();
		PrepareAndBindMaterialForRendering(commandBuffer, dxMaterial, dxCBSet);

		Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;

		ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
		ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

		ctx->RSSetState(dxPipeline->m_RasterizerState);
		ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);
		ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);

		ctx->IASetPrimitiveTopology(SharkPrimitveTopologyToD3D11(topology));

		ctx->DrawIndexed(indexCount, 0, 0);
	}

	void DirectXRenderer::Present(bool vsync)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("DirectXRenderer::Present");

		m_PresentTimer->StartQuery(m_ImmediateContext);

		IDXGISwapChain* swapChain = m_SwapChain->GetNative();
		swapChain->Present(vsync ? 1 : 0, 0);

		m_PresentTimer->EndQuery(m_ImmediateContext);


		m_SwapChain->NewFrame();
	}

	void DirectXRenderer::BindMainFrameBuffer()
	{
		SK_PROFILE_FUNCTION();
		Ref<DirectXSwapChainFrameBuffer> swapChainFrameBuffer = m_SwapChain->GetMainFrameBuffer();

		m_ImmediateContext->OMSetRenderTargets(swapChainFrameBuffer->m_Count, swapChainFrameBuffer->m_FrameBuffers.data(), swapChainFrameBuffer->m_DepthStencil);
		m_ImmediateContext->OMSetBlendState(swapChainFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);
		m_ImmediateContext->RSSetViewports(1, &swapChainFrameBuffer->m_Viewport);
	}

	void DirectXRenderer::AddRenderCommandBuffer(Weak<DirectXRenderCommandBuffer> commandBuffer)
	{
		if (s_Instance)
			s_Instance->m_CommandBuffers.push_back(Ref(commandBuffer));
	}

	void DirectXRenderer::RemoveRenderCommandBuffer(Weak<DirectXRenderCommandBuffer> commandBuffer)
	{
		if (s_Instance)
		{
			auto i = std::find(s_Instance->m_CommandBuffers.begin(), s_Instance->m_CommandBuffers.end(), Ref(commandBuffer));
			SK_CORE_ASSERT(i != s_Instance->m_CommandBuffers.end());
			if (i != s_Instance->m_CommandBuffers.end())
				s_Instance->m_CommandBuffers.erase(i);
		}
	}

	void DirectXRenderer::Flush()
	{
		SK_PROFILE_FUNCTION();

		Timer timer;
		for (auto cmdBuffer : m_CommandBuffers)
			cmdBuffer->Flush();
		m_ImmediateContext->Flush();

		SK_CORE_INFO("DirectXRenderer::Flush: {:.5f}ms", timer.Stop());
	}

	void DirectXRenderer::PrepareAndBindMaterialForRendering(Ref<DirectXRenderCommandBuffer> renderCommandBuffer, Ref<DirectXMaterial> material, Ref<DirectXConstantBufferSet> constantBufferSet)
	{
		SK_PROFILE_FUNCTION();

		ID3D11DeviceContext* ctx = renderCommandBuffer->GetContext();

		for (auto&& [name, textureArray] : material->m_ResourceMap)
		{
			ctx->PSSetShaderResources(textureArray->m_StartOffset, textureArray->m_Count, textureArray->m_Views.data());
			ctx->PSSetSamplers(textureArray->m_StartOffset, textureArray->m_Count, textureArray->m_Samplers.data());
		}

		auto& cbMap = constantBufferSet->m_CBMap;
		Ref<DirectXConstantBufferSet> matCBSet = material->m_ConstnatBufferSet;
		for (auto& [slot, cb] : matCBSet->m_CBMap)
		{
			const auto it = cbMap.find(slot);
			Ref<DirectXConstantBuffer> c = (it != cbMap.end()) ? it->second : cb;
			ctx->VSSetConstantBuffers(c->m_Slot, 1, &c->m_ConstBuffer);
		}

	}

}