#include "skpch.h"
#include "DirectXSwapChain.h"

#include "Platform/DirectX11/DirectXRenderer.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	DirectXSwapChainFrameBuffer::DirectXSwapChainFrameBuffer(IDXGISwapChain* swapchain, const FrameBufferSpecification& specs)
		: DirectXFrameBuffer(specs, true), m_SwapChain(swapchain)
	{
		m_SwapChain->AddRef();
		CreateBuffers();
	}

	DirectXSwapChainFrameBuffer::~DirectXSwapChainFrameBuffer()
	{
		m_SwapChain->Release();
	}

	void DirectXSwapChainFrameBuffer::Resize(uint32_t width, uint32_t height)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;

		for (auto& atachment : m_Specification.Atachments)
			if (atachment.Image)
				atachment.Image->Resize(width, height);

		Release();
		CreateBuffers();
	}

	void DirectXSwapChainFrameBuffer::Release()
	{
		//m_SwapChain->Release();

		DirectXFrameBuffer::Release();
	}

	void DirectXSwapChainFrameBuffer::CreateSwapChainBuffer()
	{
		auto* dev = DirectXRenderer::GetDevice();
		auto*& framebuffer = m_FrameBuffers.emplace_back(nullptr);

		// TODO(moro): Thinck about adding Images to a Swapchain FrameBuffer
		// The content of the SwapChain propbably will never be read

		ID3D11Texture2D* backBuffer;
		SK_CHECK(m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));
		dev->CreateRenderTargetView(backBuffer, nullptr, &framebuffer);
		backBuffer->Release();
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

		auto* fac = DirectXRenderer::GetFactory();
		auto* dev = DirectXRenderer::GetDevice();
		SK_CHECK(fac->CreateSwapChain(dev, &scd, &m_SwapChain));

		FrameBufferSpecification fb;
		fb.Width = specs.Widht;
		fb.Height = specs.Height;
		fb.Atachments = { ImageFormat::SwapChain };

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
		m_FrameBuffer->Clear(DirectXRenderer::GetContext(), { 0.8f, 0.8f, 0.2f, 1.0f });
	}

	void DirectXSwapChain::Resize(uint32_t width, uint32_t height)
	{
		m_FrameBuffer->UnBind(DirectXRenderer::GetContext());
		m_FrameBuffer->Release();

		DirectXRenderer::Get()->Flush();

		DXGI_SWAP_CHAIN_DESC scd;
		SK_CHECK(m_SwapChain->GetDesc(&scd));
		SK_CHECK(m_SwapChain->ResizeBuffers(scd.BufferCount, width, height, scd.BufferDesc.Format, scd.Flags));
		m_FrameBuffer->Resize(width, height);
	}

}