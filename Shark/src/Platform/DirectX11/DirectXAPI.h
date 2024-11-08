#pragma once

#include <d3d11_1.h>
#include <dxgi.h>

namespace Shark {
	class Buffer;
	class DirectXDevice;
}

namespace Shark::DirectXAPI {

	void ReleaseObject(ID3D11DeviceChild* object);

	void CreateDeferredContext(ID3D11Device* device, ID3D11DeviceContext*& outContext);

	void SetDebugName(ID3D11DeviceChild* object, const char* debugName);
	void SetDebugName(ID3D11DeviceChild* object, std::string_view debugName);

	void CreateSwapChain(IDXGIFactory* factory, ID3D11Device* device, DXGI_SWAP_CHAIN_DESC& desc, IDXGISwapChain*& outSwapChain);

	void CreateBuffer(ID3D11Device* device, const D3D11_BUFFER_DESC& desc, const D3D11_SUBRESOURCE_DATA* subresourceData, ID3D11Buffer*& outBuffer);

	void CreateShaderResourceView(ID3D11Device* device, ID3D11Resource* resource, const D3D11_SHADER_RESOURCE_VIEW_DESC& desc, ID3D11ShaderResourceView*& outView);
	void CreateRenderTargetView(ID3D11Device* device, ID3D11Resource* resource, const D3D11_RENDER_TARGET_VIEW_DESC& desc, ID3D11RenderTargetView*& outView);
	void CreateDepthStencilView(ID3D11Device* device, ID3D11Resource* resource, const D3D11_DEPTH_STENCIL_VIEW_DESC& desc, ID3D11DepthStencilView*& outView);
	void CreateUnorderedAccessView(ID3D11Device* device, ID3D11Resource* resource, const D3D11_UNORDERED_ACCESS_VIEW_DESC& desc, ID3D11UnorderedAccessView*& outView);

	void CreateBlendState(ID3D11Device* device, const D3D11_BLEND_DESC& desc, ID3D11BlendState*& outState);
	void CreateTexture2D(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& desc, const D3D11_SUBRESOURCE_DATA* subresourceData, ID3D11Texture2D*& outTexture);
	void CreateSamplerState(ID3D11Device* device, const D3D11_SAMPLER_DESC& desc, ID3D11SamplerState*& outSampler);

	void CreateVertexShader(ID3D11Device* device, Buffer binary, ID3D11ClassLinkage* classLinkage, ID3D11VertexShader*& outShader);
	void CreatePixelShader(ID3D11Device* device, Buffer binary, ID3D11ClassLinkage* classLinkage, ID3D11PixelShader*& outShader);
	void CreateComputeShader(ID3D11Device* device, Buffer binary, ID3D11ClassLinkage* classLinkage, ID3D11ComputeShader*& outShader);

	void CreateQuery(ID3D11Device* device, D3D11_QUERY queryType, ID3D11Query*& outQuery);
	void CreateQuery(ID3D11Device* device, const D3D11_QUERY_DESC& desc, ID3D11Query*& outQuery);

	  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
	 //// API for 11.1
	////

	void CreateDeferredContext(Ref<DirectXDevice> device, ID3D11DeviceContext1*& outContext);
	void CreateBlendState(Ref<DirectXDevice> device, const D3D11_BLEND_DESC& desc, ID3D11BlendState*& outState);

}
