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

		QueryCapabilities();

		auto& window = Application::Get().GetWindow();

		SwapChainSpecifications specs;
		specs.Widht = window.GetWidth();
		specs.Height = window.GetHeight();
		specs.WindowHandle = window.GetHandle();
		m_SwapChain = Ref<DirectXSwapChain>::Create(specs);


		m_ShaderLib = Ref<ShaderLibrary>::Create();

		m_ShaderLib->Load("Resources/Shaders/Renderer2D_Quad.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_Circle.hlsl");
		m_ShaderLib->Load("Resources/Shaders/Renderer2D_Line.hlsl");

		m_ShaderLib->Load("Resources/Shaders/FullScreen.hlsl");
		m_ShaderLib->Load("Resources/Shaders/CompositWidthDepth.hlsl");
		m_ShaderLib->Load("Resources/Shaders/NegativeEffect.hlsl");
		m_ShaderLib->Load("Resources/Shaders/BlurEffect.hlsl");

		uint32_t color = 0xFFFFFFFF;
		m_WhiteTexture = Texture2D::Create(ImageFormat::RGBA8, 1, 1, &color);

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

		m_QuadVertexBuffer = Ref<DirectXVertexBuffer>::Create(layout, vertices, (uint32_t)sizeof(vertices));
		m_QuadIndexBuffer = Ref<DirectXIndexBuffer>::Create(indices, (uint32_t)(sizeof(indices) / sizeof(*indices)));

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

		SK_CORE_ASSERT(m_CommandBuffers.empty(), "All RenderCommandBuffers need to be destroy befor Renderer shuts down");

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
			for (auto cmdBuffer : m_CommandBuffers)
				cmdBuffer->OnSwapchainResize();

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

	void DirectXRenderer::RenderFullScreenQuad(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<Material> material)
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

		Ref<DirectXMaterial> dxMaterial = material.As<DirectXMaterial>();
		PrepareAndBindMaterialForRendering(dxCommandBuffer, dxMaterial, nullptr);

		ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx->DrawIndexed(6, 0, 0);
	}

	void DirectXRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("DirectXRenderer::RenderGeometry");

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

		ctx->IASetPrimitiveTopology(dxPipeline->m_PrimitveTopology);

		ctx->DrawIndexed(indexCount, 0, 0);
	}

	void DirectXRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<Material> material, Ref<ConstantBufferSet> constantBufferSet, Ref<VertexBuffer> vertexBuffer, uint32_t vertexCount)
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

		Ref<DirectXMaterial> dxMaterial = material.As<DirectXMaterial>();
		Ref<DirectXConstantBufferSet> dxCBSet = constantBufferSet.As<DirectXConstantBufferSet>();
		PrepareAndBindMaterialForRendering(commandBuffer, dxMaterial, dxCBSet);

		Ref<DirectXFrameBuffer> dxFrameBuffer = dxPipeline->m_FrameBuffer;

		ctx->OMSetRenderTargets(dxFrameBuffer->m_Count, dxFrameBuffer->m_FrameBuffers.data(), dxFrameBuffer->m_DepthStencil);
		ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);

		ctx->RSSetState(dxPipeline->m_RasterizerState);
		ctx->OMSetDepthStencilState(dxPipeline->m_DepthStencilState, 0);
		ctx->OMSetBlendState(dxFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);

		ctx->IASetPrimitiveTopology(dxPipeline->m_PrimitveTopology);

		ctx->Draw(vertexCount, 0);
	}

	void DirectXRenderer::GenerateMips(Ref<Image2D> image)
	{
		Ref<DirectXImage2D> dxImage = image.As<DirectXImage2D>();
		const auto& specs = dxImage->GetSpecification();

		if (specs.MipLevels == 1)
			return;

		D3D11_TEXTURE2D_DESC textureDesc{};
		textureDesc.Width = specs.Width;
		textureDesc.Height = specs.Height;
		textureDesc.MipLevels = specs.MipLevels;
		textureDesc.ArraySize = 1;
		textureDesc.Format = utils::ImageFormatToD3D11ForResource(specs.Format);
		textureDesc.SampleDesc.Count = 1;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		textureDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

		ID3D11Texture2D* texture = nullptr;
		SK_CHECK(m_Device->CreateTexture2D(&textureDesc, nullptr, &texture));

		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
		viewDesc.Format = textureDesc.Format;
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipLevels = -1;
		viewDesc.Texture2D.MostDetailedMip = 0;
		ID3D11ShaderResourceView* view = nullptr;
		SK_CHECK(m_Device->CreateShaderResourceView(texture, &viewDesc, &view));

		m_ImmediateContext->CopySubresourceRegion(texture, 0, 0, 0, 0, dxImage->m_Resource, 0, nullptr);
		m_ImmediateContext->GenerateMips(view);
		m_ImmediateContext->CopyResource(dxImage->m_Resource, texture);

		view->Release();
		texture->Release();
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

	void DirectXRenderer::AddCommandBuffer(const Weak<DirectXRenderCommandBuffer>& commandBuffer)
	{
		m_CommandBuffers.insert(commandBuffer);
	}

	void DirectXRenderer::RemoveCommandBuffer(const Weak<DirectXRenderCommandBuffer>& commandBuffer)
	{
		auto entry = m_CommandBuffers.find(commandBuffer);
		if (entry != m_CommandBuffers.end())
			m_CommandBuffers.erase(entry);

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

		for (auto&& [slot, data] : material->m_ConstantBufferData)
			material->m_ConstnatBufferSet->Set(slot, data.Data, data.Size);

		if (constantBufferSet)
		{
			auto& cbMap = constantBufferSet->m_CBMap;
			Ref<DirectXConstantBufferSet> matCBSet = material->m_ConstnatBufferSet;
			for (auto& [slot, cb] : matCBSet->m_CBMap)
			{
				const auto it = cbMap.find(slot);
				Ref<DirectXConstantBuffer> c = (it != cbMap.end()) ? it->second : cb;
				ctx->VSSetConstantBuffers(c->m_Slot, 1, &c->m_ConstBuffer);
			}
		}
		else
		{
			Ref<DirectXConstantBufferSet> matCBSet = material->m_ConstnatBufferSet;
			for (auto& [slot, cb] : matCBSet->m_CBMap)
				ctx->VSSetConstantBuffers(cb->m_Slot, 1, &cb->m_ConstBuffer);
		}

	}

	void DirectXRenderer::QueryCapabilities()
	{
		m_Capabilities.MaxMipLeves     = D3D11_REQ_MIP_LEVELS;
		m_Capabilities.MaxAnisotropy   = D3D11_REQ_MAXANISOTROPY;
		m_Capabilities.MinLODBias      = D3D11_MIP_LOD_BIAS_MIN;
		m_Capabilities.MaxLODBias      = D3D11_MIP_LOD_BIAS_MAX;
	}

}