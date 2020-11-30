#include "skpch.h"
#include "DirectXRenderer.h"
#include "Shark/Core/Window.h"

namespace Shark {

	Renderer* Renderer::Create( const RendererProps& properties )
	{
		return new DirectXRenderer( properties );
	}

	DirectXRenderer::DirectXRenderer( const RendererProps& props )
	{
		data.width = props.width;
		data.height = props.height;

		DXGI_SWAP_CHAIN_DESC scd = { 0 };
		scd.BufferDesc.Width = data.width;
		scd.BufferDesc.Height = data.height;
		scd.BufferDesc.RefreshRate.Denominator = 0u;
		scd.BufferDesc.RefreshRate.Numerator = 0u;
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		scd.BufferDesc.Scaling = DXGI_MODE_SCALING_CENTERED;
		scd.SampleDesc.Count = 1u;
		scd.SampleDesc.Quality = 0u;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = 1u;
		scd.OutputWindow = (HWND)props.pWindowHandle;
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
			&data.pSwapChain,
			&data.pDevice,
			nullptr,
			&data.pContex
		);

		Microsoft::WRL::ComPtr<ID3D11Resource> pBackBuffer;
		data.pSwapChain->GetBuffer( 0u,__uuidof(ID3D11Texture2D),&pBackBuffer );
		data.pDevice->CreateRenderTargetView(
			pBackBuffer.Get(),
			nullptr,
			&data.pRenderTargetView
		);

		data.pContex->OMSetRenderTargets(
			1u,
			data.pRenderTargetView.GetAddressOf(),
			nullptr
		);

		D3D11_VIEWPORT vp = { 0 };
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = (float)data.width;
		vp.Height = (float)data.height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		data.pContex->RSSetViewports( 1u,&vp );
	}

	DirectXRenderer::~DirectXRenderer()
	{
	}

	void DirectXRenderer::EndFrame()
	{
		data.pSwapChain->Present( 1u,0u );
	}

	void DirectXRenderer::ClearBuffer( const Color::F32RGBA& color )
	{
		data.pContex->ClearRenderTargetView( data.pRenderTargetView.Get(),color.rgba );
	}

}