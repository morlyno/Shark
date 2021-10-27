#include "skpch.h"
#include "DirectXRenderer.h"
#include "Shark/Core/Application.h"

#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"
#include "Platform/DirectX11/DirectXFrameBuffer.h"
#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"
#include "Platform/DirectX11/DirectXBuffers.h"

#include "Shark/Debug/Instrumentor.h"

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
		Init();
	}

	DirectXRenderer::~DirectXRenderer()
	{
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
#ifdef SK_DEBUG
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
	}

	void DirectXRenderer::ShutDown()
	{
		m_SwapChain.Release();
		if (m_Device) { m_Device->Release(); m_Device = nullptr; }
		if (m_ImmediateContext) { m_ImmediateContext->Release(); m_ImmediateContext = nullptr; }
		if (m_Factory) { m_Factory->Release(); m_Factory = nullptr; }

		s_Instance = nullptr;
	}

	void DirectXRenderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<FrameBuffer> framebuffer)
	{
		framebuffer->Bind(renderCommandBuffer);
	}

	void DirectXRenderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{

	}

	void DirectXRenderer::RenderGeometry(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<FrameBuffer> frameBuffer, Ref<Shader> shader, Ref<ConstantBufferSet> constantBufferSet, Ref<Texture2DArray> textureArray, Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer, uint32_t indexCount, PrimitveTopology topology)
	{
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

		UINT offset = 0;
		UINT stride = dxVertexBuffer->m_Layout.GetVertexSize();
		ctx->IASetVertexBuffers(0, 1, &dxVertexBuffer->m_VertexBuffer, &stride, &offset);
		ctx->IASetIndexBuffer(dxIndexBuffer->m_IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		ctx->IASetInputLayout(dxShader->m_InputLayout);

		ctx->RSSetViewports(1, &dxFrameBuffer->m_Viewport);
		ctx->IASetPrimitiveTopology(SharkPrimitveTopologyToD3D11(topology));

		ctx->DrawIndexed(indexCount, 0, 0);

	}

	void DirectXRenderer::BindMainFrameBuffer()
	{
		Ref<DirectXSwapChainFrameBuffer> swapChainFrameBuffer = m_SwapChain->GetMainFrameBuffer();

		m_ImmediateContext->OMSetRenderTargets(swapChainFrameBuffer->m_Count, swapChainFrameBuffer->m_FrameBuffers.data(), swapChainFrameBuffer->m_DepthStencil);
		m_ImmediateContext->OMSetDepthStencilState(swapChainFrameBuffer->m_DepthStencilState, 0);
		m_ImmediateContext->OMSetBlendState(swapChainFrameBuffer->m_BlendState, nullptr, 0xFFFFFFFF);
		m_ImmediateContext->RSSetViewports(1, &swapChainFrameBuffer->m_Viewport);
	}

	void DirectXRenderer::SetBlendForImgui(bool blend)
	{
		if (blend)
		{
			SK_CORE_ASSERT(m_ImGuiBlendState);
			const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
			m_ImmediateContext->OMSetBlendState(m_ImGuiBlendState, blend_factor, 0xFFFFFFFF);
		}
		else
		{
			m_ImmediateContext->OMGetBlendState(&m_ImGuiBlendState, nullptr, nullptr);
			ID3D11BlendState* nullBlend = nullptr;
			const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
			m_ImmediateContext->OMSetBlendState(nullBlend, nullptr, 0xFFFFFFFF);
		}
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
		for (auto cmdBuffer : m_CommandBuffers)
			cmdBuffer->Flush();
		m_ImmediateContext->Flush();
	}

}