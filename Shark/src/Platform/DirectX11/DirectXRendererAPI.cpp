#include "skpch.h"
#include "DirectXRendererAPI.h"
#include <stdlib.h>

#include "Platform/DirectX11/DirectXBuffers.h"
#include "Platform/DirectX11/DirectXShaders.h"
#include "Platform/DirectX11/DirectXTexture.h"
#include "Platform/DirectX11/DirectXFrameBuffer.h"
#include "Platform/DirectX11/DirectXViewport.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR(SK_STRINGIFY(call) "0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	void DirectXRendererAPI::Init(const Window& window)
	{
		SK_CORE_ASSERT(m_Device == nullptr, "RendererAPI already initialized");
		SK_CORE_INFO("Init RendererAPI");

		SK_CHECK(CreateDXGIFactory(IID_PPV_ARGS(&m_Factory)));

		IDXGIAdapter* gpu = nullptr;
		if (HRESULT hr = m_Factory->EnumAdapters(0u, &gpu); FAILED(hr))
		{
			SK_CORE_ASSERT(hr != DXGI_ERROR_INVALID_CALL);
			if (hr == DXGI_ERROR_NOT_FOUND)
			{
				SK_CORE_CRITICAL("!!! No Adapter could be found !!!");
				throw std::exception("Failed to find a GPU Adapter");
			}
		}

		{
			DXGI_ADAPTER_DESC ad;
			SK_CHECK(gpu->GetDesc(&ad));
			char gpudesc[128];
			wcstombs_s(nullptr, gpudesc, ad.Description, 128);
			SK_CORE_INFO("GPU: {0}", gpudesc);
		}

		DXGI_SWAP_CHAIN_DESC scd = {};
		scd.BufferDesc.Width = window.GetWidth();
		scd.BufferDesc.Height = window.GetHeight();
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		scd.BufferDesc.RefreshRate.Denominator = 0u;
		scd.BufferDesc.RefreshRate.Numerator = 0u;
		scd.SampleDesc.Count = 1u;
		scd.SampleDesc.Quality = 0u;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = 1u;
		scd.OutputWindow = (HWND)window.GetHandle();
		scd.Windowed = TRUE;
		scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		scd.Flags = 0u;

		UINT createdeviceFalgs = 0u;
#if SK_DEBUG
		createdeviceFalgs |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		SK_CHECK(D3D11CreateDevice(
			gpu,
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			createdeviceFalgs,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&m_Device,
			nullptr,
			&m_Context
		));

		SK_CHECK(m_Factory->CreateSwapChain(m_Device, &scd, &m_SwapChain));

		if (gpu) { gpu->Release(); gpu = nullptr; }

		
		// TODO: Make chanchable in the future
		m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


		D3D11_BLEND_DESC bd;
		bd.AlphaToCoverageEnable = false;
		bd.IndependentBlendEnable = false;
		bd.RenderTarget[0].BlendEnable = TRUE;
		bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		SK_CHECK(m_Device->CreateBlendState(&bd, &m_BlendState));

		bd.RenderTarget[0].BlendEnable = FALSE;
		SK_CHECK(m_Device->CreateBlendState(&bd, &m_BlendStateNoAlpha));

		m_Context->OMSetBlendState(m_BlendState, nullptr, 0xFFFFFFFF);
	}

	void DirectXRendererAPI::ShutDown()
	{
		if (m_Device) { m_Device->Release(); m_Device = nullptr; }
		if (m_Context) { m_Context->Release(); m_Context = nullptr; }
		if (m_SwapChain) { m_SwapChain->Release(); m_SwapChain = nullptr; }
		if (m_Factory) { m_Factory->Release(); m_Factory = nullptr; }
		if (m_BlendState) { m_BlendState->Release(); m_BlendState = nullptr; }
		if (m_BlendStateNoAlpha) { m_BlendStateNoAlpha->Release(); m_BlendStateNoAlpha = nullptr; }
	}

	void DirectXRendererAPI::SwapBuffer(bool VSync)
	{
		SK_CHECK(m_SwapChain->Present(VSync ? 1u : 0u, 0u));
	}

	void DirectXRendererAPI::SetBlendState(bool blend)
	{
		if (blend)
			m_Context->OMSetBlendState(m_BlendState, nullptr, 0xFFFFFFFF);
		else
			m_Context->OMSetBlendState(m_BlendStateNoAlpha, nullptr, 0xFFFFFFFF);
	}

	void DirectXRendererAPI::DrawIndexed(uint32_t count)
	{
		m_Context->DrawIndexed(count, 0u, 0u);
	}

	void DirectXRendererAPI::Flush()
	{
		m_Context->Flush();
	}

	Ref<VertexBuffer> DirectXRendererAPI::CreateVertexBuffer(const VertexLayout& layout, bool dynamic)
	{
		return CreateRef<DirectXVertexBuffer>(layout, dynamic, APIContext{ m_Device, m_Context });
	}

	Ref<VertexBuffer> DirectXRendererAPI::CreateVertexBuffer(const VertexLayout& layout, void* data, uint32_t size, bool dynamic)
	{
		return CreateRef<DirectXVertexBuffer>(layout, data, size, dynamic, APIContext{ m_Device, m_Context });
	}

	Ref<IndexBuffer> DirectXRendererAPI::CreateIndexBuffer(uint32_t* indices, uint32_t count)
	{
		return CreateRef<DirectXIndexBuffer>(indices, count, APIContext{ m_Device, m_Context });
	}

	Ref<Shaders> DirectXRendererAPI::CreateShaders(const std::string& filepath)
	{
		return CreateRef<DirectXShaders>(filepath, APIContext{ m_Device, m_Context });
	}

	Ref<Shaders> DirectXRendererAPI::CreateShaders(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc)
	{
		return CreateRef<DirectXShaders>(vertexshaderSrc, pixelshaderSrc, APIContext{ m_Device, m_Context });
	}

	Ref<Texture2D> DirectXRendererAPI::CreateTexture2D(const SamplerSpecification& sampler, const std::string& filepath)
	{
		return CreateRef<DirectXTexture2D>(sampler, filepath, APIContext{ m_Device, m_Context });
	}

	Ref<Texture2D> DirectXRendererAPI::CreateTexture2D(const SamplerSpecification& sampler, uint32_t width, uint32_t height, uint32_t flatcolor)
	{
		return CreateRef<DirectXTexture2D>(sampler, width, height, flatcolor, APIContext{ m_Device, m_Context });
	}
	
	Ref<Texture2D> DirectXRendererAPI::CreateTexture2D(const SamplerSpecification& sampler, uint32_t width, uint32_t height, void* data)
	{
		return CreateRef<DirectXTexture2D>(sampler, width, height, data, APIContext{ m_Device, m_Context });
	}

	Ref<FrameBuffer> DirectXRendererAPI::CreateFrameBuffer(const FrameBufferSpecification& specs)
	{
		return CreateRef<DirectXFrameBuffer>(specs, APIContext{ m_Device, m_Context }, m_SwapChain);
	}

	Ref<Viewport> DirectXRendererAPI::CreateViewport(uint32_t width, uint32_t height)
	{
		return CreateRef<DirectXViewport>(width, height, APIContext{ m_Device, m_Context });
	}

}