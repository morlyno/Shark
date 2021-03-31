#include "skpch.h"
#include "DirectXSwapChain.h"

#include "Shark/Render/RendererCommand.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	DirectXSwapChain::DirectXSwapChain(const SwapChainSpecifications& specs)
		: m_BufferCount(specs.BufferCount)
	{
		m_DXApi = RendererCommand::GetRendererAPI().CastTo<DirectXRendererAPI>();

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

		SK_CHECK(m_DXApi->GetFactory()->CreateSwapChain(m_DXApi->GetDevice(), &scd, &m_SwapChain));
	}

	DirectXSwapChain::~DirectXSwapChain()
	{
		if (m_SwapChain)
			m_SwapChain->Release();
	}

	void DirectXSwapChain::SwapBuffers(bool vsync)
	{
		SK_CHECK(m_SwapChain->Present(vsync ? 1 : 0, 0));
	}

	void DirectXSwapChain::Resize(uint32_t width, uint32_t height)
	{
		DXGI_SWAP_CHAIN_DESC scd;
		SK_CHECK(m_SwapChain->GetDesc(&scd));
		SK_CHECK(m_SwapChain->ResizeBuffers(scd.BufferCount, width, height, scd.BufferDesc.Format, scd.Flags));
	}

	void DirectXSwapChain::GetBackBuffer(uint32_t index, ID3D11Texture2D** buffer)
	{
		SK_CHECK(m_SwapChain->GetBuffer(index, __uuidof(ID3D11Texture2D), (void**)buffer));
	}

}