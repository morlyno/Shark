#include "skpch.h"
#include "DirectXRendererAPI.h"
#include <stdlib.h>


namespace Shark {

	void DirectXRendererAPI::Init(const Window& window)
	{
		SK_CORE_ASSERT(m_Device == nullptr, "RendererAPI already initialized");
		SK_CORE_INFO("Init RendererAPI");

		if (CreateDXGIFactory(IID_PPV_ARGS(&m_Factory)) != S_OK)
			SK_CORE_ASSERT(false, "Create DXGI Factore Failed");

		IDXGIAdapter* gpu = nullptr;
#if 0
		if (HRESULT hr = m_Factory->EnumAdapters(1u, &gpu); FAILED(hr))
		{
			SK_CORE_ASSERT(hr != DXGI_ERROR_INVALID_CALL);
			if (hr == DXGI_ERROR_NOT_FOUND)
			{
				SK_CORE_WARN("Adapter not found default Adapter seleted instead");
				if (m_Factory->EnumAdapters(0u, &gpu) == DXGI_ERROR_NOT_FOUND)
					SK_CORE_STOP_APPLICATION("!!! No Adapter could be found !!!");
			}
		}
#endif
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
			gpu->GetDesc(&ad);
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

		D3D11CreateDeviceAndSwapChain(
			gpu,
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			createdeviceFalgs,
			nullptr,
			0u,
			D3D11_SDK_VERSION,
			&scd,
			&m_SwapChain,
			&m_Device,
			nullptr,
			&m_Context
		);

		if (gpu) { gpu->Release(); gpu = nullptr; }

		D3D11_DEPTH_STENCIL_DESC ds = {};
		ds.DepthEnable = TRUE;
		ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		ds.DepthFunc = D3D11_COMPARISON_LESS;

		ID3D11DepthStencilState* depthState = nullptr;
		m_Device->CreateDepthStencilState(&ds, &depthState);
		m_Context->OMSetDepthStencilState(depthState, 1u);
		depthState->Release();


		D3D11_DEPTH_STENCIL_VIEW_DESC dsv;
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0u;
		dsv.Flags = 0u;

		D3D11_TEXTURE2D_DESC t2d;
		t2d.Width = window.GetWidth();
		t2d.Height = window.GetHeight();
		t2d.MipLevels = 1u;
		t2d.ArraySize = 1u;
		t2d.Format = dsv.Format;
		t2d.SampleDesc.Count = 1u;
		t2d.SampleDesc.Quality = 0u;
		t2d.Usage = D3D11_USAGE_DEFAULT;
		t2d.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		t2d.CPUAccessFlags = 0u;
		t2d.MiscFlags = 0u;

		ID3D11Texture2D* texture = nullptr;
		m_Device->CreateTexture2D(&t2d, nullptr, &texture);

		m_Device->CreateDepthStencilView(texture, &dsv, &m_DepthStencil);
		texture->Release();

		ID3D11Texture2D* buffer;
		m_SwapChain->GetBuffer(0u, __uuidof(ID3D11Texture2D), (void**)&buffer);
		m_Device->CreateRenderTargetView(buffer, nullptr, &m_RenderTarget);
		m_Context->OMSetRenderTargets(1u, &m_RenderTarget, m_DepthStencil);
		buffer->Release();

		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0u;
		vp.TopLeftY = 0u;
		vp.Width = (float)window.GetWidth();
		vp.Height = (float)window.GetHeight();
		vp.MinDepth = 0u;
		vp.MaxDepth = 1u;
		m_Context->RSSetViewports(1u, &vp);

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
		m_Device->CreateBlendState(&bd, &m_BlendState);

		bd.RenderTarget[0].BlendEnable = FALSE;
		m_Device->CreateBlendState(&bd, &m_BlendStateNoAlpha);

		m_Context->OMSetBlendState(m_BlendState, nullptr, 0xFFFFFFFF);
		m_BlendEnabled = true;
	}

	void DirectXRendererAPI::ShutDown()
	{
		if (m_Device) { m_Device->Release(); m_Device = nullptr; }
		if (m_Context) { m_Context->Release(); m_Context = nullptr; }
		if (m_SwapChain) { m_SwapChain->Release(); m_SwapChain = nullptr; }
		if (m_RenderTarget) { m_RenderTarget->Release(); m_RenderTarget = nullptr; }
		if (m_Factory) { m_Factory->Release(); m_Factory = nullptr; }
		if (m_BlendState) { m_BlendState->Release(); m_BlendState = nullptr; }
		if (m_BlendStateNoAlpha) { m_BlendStateNoAlpha->Release(); m_BlendStateNoAlpha = nullptr; }
		if (m_DepthStencil) { m_DepthStencil->Release(); m_DepthStencil = nullptr; }
	}

	void DirectXRendererAPI::GetFramebufferContent(const Ref<Texture2D>& texture)
	{
		ID3D11Texture2D* backbuffer;
		m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer);

		D3D11_TEXTURE2D_DESC t2ddesc;
		backbuffer->GetDesc(&t2ddesc);
		t2ddesc.BindFlags = 0;
		t2ddesc.Usage = D3D11_USAGE_STAGING;
		t2ddesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		ID3D11Texture2D* TempData;
		m_Device->CreateTexture2D(&t2ddesc, nullptr, &TempData);

		m_Context->CopyResource(TempData, backbuffer);
		D3D11_MAPPED_SUBRESOURCE ms;
		m_Context->Map(TempData, 0, D3D11_MAP_READ, 0, &ms);

		void* alignedData = new char[(uint64_t)t2ddesc.Width * t2ddesc.Height * 4];
		uint8_t* dest = (uint8_t*)alignedData;
		const uint8_t* src = (uint8_t*)ms.pData;
		const uint32_t destPitch = t2ddesc.Width * 4;
		const uint32_t srcPitch = ms.RowPitch;
		for (uint32_t cnt = 0; cnt < t2ddesc.Height; ++cnt)
		{
			memcpy(dest, src, destPitch);
			dest += destPitch;
			src += srcPitch;
		}
		texture->SetData(alignedData);
		delete[] alignedData;

		backbuffer->Release();
		TempData->Release();
	}

	void DirectXRendererAPI::SetClearColor(const float color[4])
	{
		memcpy(m_ClearColor, color, sizeof(float) * 4);
	}

	void DirectXRendererAPI::ClearBuffer()
	{
		m_Context->OMSetRenderTargets(1u, &m_RenderTarget, m_DepthStencil);
		m_Context->ClearRenderTargetView(m_RenderTarget, m_ClearColor);
		m_Context->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_DEPTH, 1u, 0u);
	};

	void DirectXRendererAPI::SwapBuffer(bool VSync)
	{
		m_SwapChain->Present(VSync ? 1u : 0u, 0u);
	}

	void DirectXRendererAPI::SetBlendState(bool blend)
	{
		m_BlendEnabled = blend;
		if (blend)
			m_Context->OMSetBlendState(m_BlendState, nullptr, 0xFFFFFFFF);
		else
			m_Context->OMSetBlendState(m_BlendStateNoAlpha, nullptr, 0xFFFFFFFF);
	}

	void DirectXRendererAPI::DrawIndexed(uint32_t count)
	{
		m_Context->DrawIndexed(count, 0u, 0u);
	}

	void DirectXRendererAPI::OnResize(int width, int height)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC dsv;
		m_DepthStencil->GetDesc(&dsv);

		ID3D11Texture2D* resource;
		m_DepthStencil->GetResource((ID3D11Resource**)&resource);

		D3D11_TEXTURE2D_DESC t2d;
		resource->GetDesc(&t2d);
		t2d.Width = width;
		t2d.Height = height;
		resource->Release();


		m_Context->OMSetRenderTargets(0u, nullptr, nullptr);
		m_RenderTarget->Release();
		m_DepthStencil->Release();

		m_SwapChain->ResizeBuffers(0u, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0u);

		ID3D11Texture2D* texture;
		m_Device->CreateTexture2D(&t2d, nullptr, &texture);
		m_Device->CreateDepthStencilView(texture, &dsv, &m_DepthStencil);
		texture->Release();

		ID3D11Texture2D* buffer = nullptr;
		m_SwapChain->GetBuffer(0u, __uuidof(ID3D11Texture2D), (void**)&buffer);
		m_Device->CreateRenderTargetView(buffer, nullptr, &m_RenderTarget);
		m_Context->OMSetRenderTargets(1u, &m_RenderTarget, m_DepthStencil);
		buffer->Release();

		D3D11_VIEWPORT vp;
		vp.TopLeftX = 0u;
		vp.TopLeftY = 0u;
		vp.Width = (float)width;
		vp.Height = (float)height;
		vp.MinDepth = 0u;
		vp.MaxDepth = 1u;
		m_Context->RSSetViewports(1u, &vp);
	};

}