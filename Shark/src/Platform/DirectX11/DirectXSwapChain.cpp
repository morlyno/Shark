#include "skpch.h"
#include "DirectXSwapChain.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	DirectXSwapChainFrameBuffer::DirectXSwapChainFrameBuffer(IDXGISwapChain* swapchain, const SwapChainFrameBufferSpecification& specs)
		: m_Specification(specs)
	{
		Create(swapchain);
	}

	DirectXSwapChainFrameBuffer::~DirectXSwapChainFrameBuffer()
	{
		Release();
	}

	void DirectXSwapChainFrameBuffer::Release()
	{
		if (m_FrameBuffer)
			m_FrameBuffer->Release();
		if (m_BlendState)
			m_BlendState->Release();
		if (m_DepthStencil)
			m_DepthStencil->Release();
		if (m_DepthStencilState)
			m_DepthStencilState->Release();
		memset(&m_Viewport, 0, sizeof(D3D11_VIEWPORT));
		m_FrameBuffer = nullptr;
		m_BlendState = nullptr;
		m_DepthStencil = nullptr;
		m_DepthStencilState = nullptr;
	}

	void DirectXSwapChainFrameBuffer::Resize(IDXGISwapChain* swapchain, uint32_t width, uint32_t height)
	{
		Release();
		Create(swapchain);
	}


	void DirectXSwapChainFrameBuffer::Clear(Utility::ColorF32 clearcolor)
	{
		auto ctx = DirectXRendererAPI::GetContext();

		ctx->ClearRenderTargetView(m_FrameBuffer, clearcolor.rgba);
		ctx->ClearDepthStencilView(m_DepthStencil, D3D11_CLEAR_DEPTH, 1u, 0u);
	}

	void DirectXSwapChainFrameBuffer::SetBlend(bool blend)
	{
		if (m_Specification.Blend == blend)
			return;

		m_Specification.Blend = blend;

		D3D11_BLEND_DESC bd;
		m_BlendState->GetDesc(&bd);
		bd.RenderTarget[0].BlendEnable = blend;
		if (blend)
		{
			bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		}

		m_BlendState->Release();
		SK_CHECK(DirectXRendererAPI::GetDevice()->CreateBlendState(&bd, &m_BlendState));
		DirectXRendererAPI::GetContext()->OMSetBlendState(m_BlendState, nullptr, 0xffffffff);
	}

	bool DirectXSwapChainFrameBuffer::GetBlend() const
	{
		return m_Specification.Blend;
	}

	void DirectXSwapChainFrameBuffer::SetDepth(bool enabled)
	{
		if (m_DepthEnabled == enabled)
			return;
		m_DepthEnabled = enabled;

		D3D11_DEPTH_STENCIL_DESC dsd;
		m_DepthStencilState->GetDesc(&dsd);
		dsd.DepthEnable = enabled;
		m_DepthStencilState->Release();
		SK_CHECK(DirectXRendererAPI::GetDevice()->CreateDepthStencilState(&dsd, &m_DepthStencilState));
	}

	bool DirectXSwapChainFrameBuffer::GetDepth() const
	{
		return m_DepthEnabled;
	}

	void DirectXSwapChainFrameBuffer::Bind()
	{
		auto* ctx = DirectXRendererAPI::GetContext();

		ctx->OMSetDepthStencilState(m_DepthStencilState, 1);
		ctx->OMSetRenderTargets(1, &m_FrameBuffer, m_DepthStencil);
		ctx->OMSetBlendState(m_BlendState, nullptr, 0xffffffff);
		ctx->RSSetViewports(1, &m_Viewport);
	}

	void DirectXSwapChainFrameBuffer::UnBind()
	{
		auto* ctx = DirectXRendererAPI::GetContext();

		ctx->OMSetDepthStencilState(nullptr, 0);
		ctx->OMSetRenderTargets(0, nullptr, nullptr);
		ctx->OMSetBlendState(nullptr, nullptr, 0xffffffff);
		ctx->RSSetViewports(0, nullptr);
	}

	void DirectXSwapChainFrameBuffer::Create(IDXGISwapChain* swapchain)
	{
		auto dev = DirectXRendererAPI::GetDevice();

		m_Viewport.TopLeftX = 0;
		m_Viewport.TopLeftY = 0;
		m_Viewport.Width = m_Specification.Width;
		m_Viewport.Height = m_Specification.Height;
		m_Viewport.MinDepth = 0;
		m_Viewport.MaxDepth = 1;

		ID3D11Texture2D* backBuffer;
		SK_CHECK(swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));
		dev->CreateRenderTargetView(backBuffer, nullptr, &m_FrameBuffer);

		if (m_Specification.DepthAtachment != DepthAtachment::None)
		{
			m_DepthEnabled = true;
			SK_CORE_ASSERT(m_Specification.DepthAtachment == DepthAtachment::Depth32);
			D3D11_DEPTH_STENCIL_DESC ds = {};
			ds.DepthEnable = TRUE;
			ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			ds.DepthFunc = D3D11_COMPARISON_LESS;

			SK_CHECK(dev->CreateDepthStencilState(&ds, &m_DepthStencilState));


			D3D11_DEPTH_STENCIL_VIEW_DESC dsv;
			dsv.Format = DXGI_FORMAT_D32_FLOAT;
			dsv.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsv.Texture2D.MipSlice = 0u;
			dsv.Flags = 0u;

			D3D11_TEXTURE2D_DESC t2d;
			t2d.Width = m_Specification.Width;
			t2d.Height = m_Specification.Height;
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
			SK_CHECK(dev->CreateTexture2D(&t2d, nullptr, &texture));
			SK_CHECK(dev->CreateDepthStencilView(texture, &dsv, &m_DepthStencil));
			texture->Release();
		}

		D3D11_BLEND_DESC bd;
		bd.AlphaToCoverageEnable = false;
		bd.IndependentBlendEnable = false;
		bd.RenderTarget[0].BlendEnable = m_Specification.Blend;
		bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		dev->CreateBlendState(&bd, &m_BlendState);

	}


	DirectXSwapChain::DirectXSwapChain(const SwapChainSpecifications& specs)
		: m_BufferCount(specs.BufferCount)
	{
		SK_CORE_ASSERT(specs.BufferCount == 1, "Multi buffering not implemented");
		DXGI_SWAP_CHAIN_DESC scd;
		memset(&scd, 0, sizeof(DXGI_SWAP_CHAIN_DESC));
		scd.BufferDesc.Width = specs.Widht;
		scd.BufferDesc.Height = specs.Height;
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		scd.BufferDesc.RefreshRate.Denominator = 0u;
		scd.BufferDesc.RefreshRate.Numerator = 0u;
		scd.SampleDesc.Count = 1u;
		scd.SampleDesc.Quality = 0u;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = specs.BufferCount;
		scd.OutputWindow = (HWND)specs.WindowHandle;
		scd.Windowed = TRUE;
		scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		scd.Flags = 0u;

		auto* fac = DirectXRendererAPI::GetFactory();
		auto* dev = DirectXRendererAPI::GetDevice();
		SK_CHECK(fac->CreateSwapChain(dev, &scd, &m_SwapChain));

		SwapChainFrameBufferSpecification fb;
		fb.Width = specs.Widht;
		fb.Height = specs.Height;
		fb.Blend = true;
		fb.DepthAtachment = DepthAtachment::Depth;
		m_FrameBuffer = Ref<DirectXSwapChainFrameBuffer>::Create(m_SwapChain, fb);
	}

	DirectXSwapChain::~DirectXSwapChain()
	{
		if (m_SwapChain)
			m_SwapChain->Release();
	}

	void DirectXSwapChain::SwapBuffers(bool vsync)
	{
		SK_CHECK(m_SwapChain->Present(vsync ? 1 : 0, 0));
		m_FrameBuffer->Clear({ 0.8f, 0.8f, 0.2f, 1.0f });
	}

	void DirectXSwapChain::Resize(uint32_t width, uint32_t height)
	{
		m_FrameBuffer->Release();
		DXGI_SWAP_CHAIN_DESC scd;
		SK_CHECK(m_SwapChain->GetDesc(&scd));
		SK_CHECK(m_SwapChain->ResizeBuffers(scd.BufferCount, width, height, scd.BufferDesc.Format, scd.Flags));
		m_FrameBuffer->Resize(m_SwapChain, width, height);
	}

}