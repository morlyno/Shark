#include "skpch.h"
#include "DirectXRendererAPI.h"

namespace Shark {

	void DirectXRendererAPI::Init( const Window& window )
	{
		SK_CORE_ASSERT( m_Device == nullptr,"RendererAPI already initialized" );

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
 
		UINT flags = 0u;
#ifdef SK_DEBUG
		flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			flags,
			nullptr,
			0u,
			D3D11_SDK_VERSION,
			&scd,
			&m_SwapChain,
			&m_Device,
			nullptr,
			&m_Context
		);

		ID3D11Resource* buffer;
		m_SwapChain->GetBuffer( 0u,__uuidof(ID3D11Texture2D),(void**)&buffer );
		m_Device->CreateRenderTargetView( buffer,nullptr,&m_RenderTarget );
		m_Context->OMSetRenderTargets( 1u,&m_RenderTarget,nullptr );
		buffer->Release();
		
		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0u;
		vp.TopLeftY = 0u;
		vp.Width = (float)window.GetWidth();
		vp.Height = (float)window.GetHeight();
		vp.MinDepth = 0u;
		vp.MaxDepth = 1u;
		m_Context->RSSetViewports( 1u,&vp );

		// TODO: Make chanchable in the future
		m_Context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	}

	void DirectXRendererAPI::ShutDown()
	{
		if (m_Device) { m_Device->Release();         m_Device = nullptr; }
		if (m_Context) { m_Context->Release();        m_Context = nullptr; }
		if (m_SwapChain) { m_SwapChain->Release();      m_SwapChain = nullptr; }
		if (m_RenderTarget) { m_RenderTarget->Release();   m_RenderTarget = nullptr; }
	}

	DirectXRendererAPI::~DirectXRendererAPI()
	{
		ShutDown();
	}

	void DirectXRendererAPI::SetClearColor( const float color[4] )
	{
		memcpy( clear_color,color,sizeof( float ) * 4 );
	}

	void DirectXRendererAPI::ClearBuffer()
	{
		m_Context->OMSetRenderTargets( 1u,&m_RenderTarget,nullptr );
		m_Context->ClearRenderTargetView( m_RenderTarget,clear_color );
	}

	void DirectXRendererAPI::SwapBuffer( bool VSync )
	{
		m_SwapChain->Present( VSync ? 1u : 0u,0u );
	}

	void DirectXRendererAPI::DrawIndexed( uint32_t count )
	{
		m_Context->DrawIndexed( count,0u,0u );
	}

	void DirectXRendererAPI::OnResize( int width,int height )
	{
		if ( width <= 0 || height <= 0 )
			return;

		m_Context->OMSetRenderTargets( 0u,nullptr,nullptr );
		m_RenderTarget->Release();

		m_SwapChain->ResizeBuffers( 0u,width,height,DXGI_FORMAT_R8G8B8A8_UNORM,0u );

		ID3D11Buffer* buffer = nullptr;
		m_SwapChain->GetBuffer( 0u,__uuidof(ID3D11Texture2D),(void**)&buffer );
		m_Device->CreateRenderTargetView( buffer,nullptr,&m_RenderTarget );
		m_Context->OMSetRenderTargets( 1u,&m_RenderTarget,nullptr );
		buffer->Release();

		D3D11_VIEWPORT vp;
		vp.TopLeftX = 0u;
		vp.TopLeftY = 0u;
		vp.Width = (float)width;
		vp.Height = (float)height;
		vp.MinDepth = 0u;
		vp.MaxDepth = 1u;
		m_Context->RSSetViewports( 1u,&vp );
	};

}