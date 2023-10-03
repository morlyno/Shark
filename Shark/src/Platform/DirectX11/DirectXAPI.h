#pragma once

#include <d3d11.h>
#include <dxgi.h>

namespace Shark::DirectXAPI {

	void SetDebugName(ID3D11DeviceChild* object, const char* debugName);

	void CreateSwapChain(IDXGIFactory* factory, ID3D11Device* device, DXGI_SWAP_CHAIN_DESC& desc, IDXGISwapChain*& outSwapChain);

	void CreateShaderResourceView(ID3D11Device* device, ID3D11Resource* resource, const D3D11_SHADER_RESOURCE_VIEW_DESC& desc, ID3D11ShaderResourceView*& outView);
	void CreateRenderTargetView(ID3D11Device* device, ID3D11Resource* resource, const D3D11_RENDER_TARGET_VIEW_DESC& desc, ID3D11RenderTargetView*& outView);
	void CreateDepthStencilView(ID3D11Device* device, ID3D11Resource* resource, const D3D11_DEPTH_STENCIL_VIEW_DESC& desc, ID3D11DepthStencilView*& outView);

	void CreateBlendState(ID3D11Device* device, D3D11_BLEND_DESC& desc, ID3D11BlendState*& outState);
	void CreateTexture2D(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& desc, const D3D11_SUBRESOURCE_DATA* subresourceData, ID3D11Texture2D*& outTexture);

}
