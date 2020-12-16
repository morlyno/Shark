#include "skpch.h"
#include "DirectXRenderer.h"
#include "Shark/Core/Window.h"

namespace Shark {

	Renderer* Renderer::Create( const Window* window )
	{
		return new DirectXRenderer( window );
	}

	DirectXRenderer::DirectXRenderer( const Window* window )
		:
		m_Width( window->GetWidth() ),
		m_Height( window->GetHeight() ),
		m_VSync( true )
	{
		SK_CORE_INFO( "Init DirectX Renderer" );

		DXGI_SWAP_CHAIN_DESC scd = { 0 };
		scd.BufferDesc.Width = window->GetWidth();
		scd.BufferDesc.Height = window->GetHeight();
		scd.BufferDesc.RefreshRate.Denominator = 0u;
		scd.BufferDesc.RefreshRate.Numerator = 0u;
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		scd.SampleDesc.Count = 1u;
		scd.SampleDesc.Quality = 0u;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = 1u;
		scd.OutputWindow = (HWND)window->GetHandle();
		scd.Windowed = TRUE;
		scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		scd.Flags = 0u;

#ifdef SK_DEBUG
		UINT CreateDeviceflags = D3D11_CREATE_DEVICE_DEBUG;
#else
		UINT CreateDeviceflags = 0u;
#endif

		D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			CreateDeviceflags,
			nullptr,
			0u,
			D3D11_SDK_VERSION,
			&scd,
			&m_SwapChain,
			&m_Device,
			nullptr,
			&m_Context
		);

		Microsoft::WRL::ComPtr<ID3D11Resource> pBackBuffer;
		m_SwapChain->GetBuffer( 0u,__uuidof(ID3D11Texture2D),&pBackBuffer );
		m_Device->CreateRenderTargetView(
			pBackBuffer.Get(),
			nullptr,
			&m_RenderTargetView
		);

		m_Context->OMSetRenderTargets(
			1u,
			m_RenderTargetView.GetAddressOf(),
			nullptr
		);

		D3D11_VIEWPORT vp = { 0 };
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = (float)window->GetWidth();
		vp.Height = (float)window->GetHeight();
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		m_Context->RSSetViewports( 1u,&vp );
	}

	DirectXRenderer::~DirectXRenderer()
	{
		SK_CORE_INFO( "DirectXRenderer detor" );
	}

	void DirectXRenderer::PresentFrame()
	{
		m_SwapChain->Present( m_VSync ? 1u : 0u,0u );
	}

	void DirectXRenderer::ClearBuffer( const Color::F32RGBA& color )
	{
		m_Context->OMSetRenderTargets( 1u,m_RenderTargetView.GetAddressOf(),nullptr );
		m_Context->ClearRenderTargetView( m_RenderTargetView.Get(),color.rgba );
	}

	void DirectXRenderer::OnResize( int width,int height )
	{
		m_Width = width;
		m_Height = height;

		// TODO:
		m_RenderTargetView->Release();
		
		m_SwapChain->ResizeBuffers(
			0u,
			(UINT)width,
			(UINT)height,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			0u
		);

		Microsoft::WRL::ComPtr<ID3D11Texture2D> pBackBuffer;
		m_SwapChain->GetBuffer( 0u,__uuidof(ID3D11Texture2D),&pBackBuffer );
		m_Device->CreateRenderTargetView(
			pBackBuffer.Get(),
			nullptr,
			&m_RenderTargetView
		);

		// --- delete --- //

		m_Context->OMSetRenderTargets( 1u,m_RenderTargetView.GetAddressOf(),nullptr );

		D3D11_VIEWPORT vp = { 0 };
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = (float)m_Width;
		vp.Height = (float)m_Height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		m_Context->RSSetViewports( 1u,&vp );

		// --- ______ --- //
	}

}