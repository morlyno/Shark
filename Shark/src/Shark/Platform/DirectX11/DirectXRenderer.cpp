#include "skpch.h"
#include "DirectXRenderer.h"
#include "Shark/Core/Window.h"

namespace Shark {

	Renderer* Renderer::Create( const Window& window )
	{
		return new DirectXRenderer( window );
	}

	DirectXRenderer::DirectXRenderer( const Window& window )
	{
		SK_CORE_INFO( "Init DirectX Renderer" );

		m_Width = window.GetWidth();
		m_Height = window.GetHeight();

		DXGI_SWAP_CHAIN_DESC scd = { 0 };
		scd.BufferDesc.Width = window.GetWidth();
		scd.BufferDesc.Height = window.GetHeight();
		scd.BufferDesc.RefreshRate.Denominator = 0u;
		scd.BufferDesc.RefreshRate.Numerator = 0u;
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		scd.BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
		scd.SampleDesc.Count = 1u;
		scd.SampleDesc.Quality = 0u;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = 1u;
		scd.OutputWindow = (HWND)window.GetHandle();
		scd.Windowed = TRUE;
		scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		scd.Flags = 0u;

		D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0u,
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
		vp.Width = (float)window.GetWidth();
		vp.Height = (float)window.GetHeight();
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		m_Context->RSSetViewports( 1u,&vp );
	}

	DirectXRenderer::~DirectXRenderer()
	{
	}

	void DirectXRenderer::PresentFrame()
	{
		m_SwapChain->Present( 1u,0u );
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

		// --- delete --- //
		m_Context->OMSetRenderTargets( 0,0,0 );
		// --- ______ --- //
		m_RenderTargetView->Release();
		
		m_SwapChain->ResizeBuffers(
			0u,
			(UINT)width,
			(UINT)height,
			DXGI_FORMAT_UNKNOWN,
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
		m_Context->OMSetRenderTargets(
			1u,
			m_RenderTargetView.GetAddressOf(),
			nullptr
		);

		D3D11_VIEWPORT vp = { 0 };
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = (float)width;
		vp.Height = (float)height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		m_Context->RSSetViewports( 1u,&vp );
		// --- ______ --- //
	}

	void DirectXRenderer::InitDrawTrinagle()
	{
		// Vertexbuffer

		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = sizeof( vertecies );
		bd.StructureByteStride = sizeof( Vertex );
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0u;
		bd.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA srd = {};
		srd.pSysMem = vertecies;

		m_Device->CreateBuffer( &bd,&srd,&VertexBuffer );

		// Shader

		Microsoft::WRL::ComPtr<ID3DBlob> VSblob;
		D3DReadFileToBlob( L"../bin/Debug-windows-x86_64/Shark/VertexShader.cso",&VSblob );
		m_Device->CreateVertexShader( VSblob->GetBufferPointer(),VSblob->GetBufferSize(),nullptr,&VertexShader );

		Microsoft::WRL::ComPtr<ID3DBlob> PSblob;
		D3DReadFileToBlob( L"../bin/Debug-windows-x86_64/Shark/PixelShader.cso",&PSblob );
		m_Device->CreatePixelShader( PSblob->GetBufferPointer(),PSblob->GetBufferSize(),nullptr,&PixelShader );

		// Input Layout

		const D3D11_INPUT_ELEMENT_DESC ied[] =
		{
			{ "Position",0u,DXGI_FORMAT_R32G32B32_FLOAT,0u,0u,D3D11_INPUT_PER_VERTEX_DATA,0u }
		};
		m_Device->CreateInputLayout( ied,(UINT)std::size( ied ),VSblob->GetBufferPointer(),VSblob->GetBufferSize(),&InputLayout );
	}

	void DirectXRenderer::DrawTriangle()
	{
		// Vertexbuffer

		const UINT stride = sizeof( Vertex );
		const UINT offset = 0u;
		m_Context->IASetVertexBuffers( 0u,1u,VertexBuffer.GetAddressOf(),&stride,&offset );

		// Shader

		m_Context->VSSetShader( VertexShader.Get(),nullptr,0u );

		m_Context->PSSetShader( PixelShader.Get(),nullptr,0u );

		// Input layout

		m_Context->IASetInputLayout( InputLayout.Get() );

		// Topology

		m_Context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

		// Draw

		m_Context->Draw( (UINT)std::size( vertecies ),0u );
	}

}