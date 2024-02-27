#include "skpch.h"
#include "DirectXAPI.h"

#include "Platform/DirectX11/DirectXRenderer.h"

#define DX_CHECK_RESULT(_hResult) ::Shark::DirectXAPI::HandleResult(_hResult)

namespace Shark::DirectXAPI {

	static void HandleResult(HRESULT hr)
	{
		if (FAILED(hr))
		{
			auto renderer = DirectXRenderer::Get();
			renderer->HandleError(hr);
		}
	}

	void ReleaseObject(ID3D11DeviceChild* object)
	{
		if (object)
		{
			object->Release();
		}
	}

	void CreateDeferredContext(ID3D11Device* device, ID3D11DeviceContext*& outContext)
	{
		HRESULT hr = device->CreateDeferredContext(0, &outContext);
		DX_CHECK_RESULT(hr);
	}

	void SetDebugName(ID3D11DeviceChild* object, const char* debugName)
	{
		HRESULT hr = D3D_SET_OBJECT_NAME_A(object, debugName);
		if (FAILED(hr))
		{
			auto renderer = DirectXRenderer::Get();
			renderer->HandleError(hr);
		}
	}

	void SetDebugName(ID3D11DeviceChild* object, std::string_view debugName)
	{
		HRESULT hr = D3D_SET_OBJECT_NAME_N_A(object, (UINT)debugName.length(), debugName.data());
		if (FAILED(hr))
		{
			auto renderer = DirectXRenderer::Get();
			renderer->HandleError(hr);
		}
	}

	void CreateSwapChain(IDXGIFactory* factory, ID3D11Device* device, DXGI_SWAP_CHAIN_DESC& desc, IDXGISwapChain*& outSwapChain)
	{
		HRESULT hr = factory->CreateSwapChain(device, &desc, &outSwapChain);
		if (FAILED(hr))
		{
			auto renderer = DirectXRenderer::Get();
			renderer->HandleError(hr);
		}
	}

	void CreateBuffer(ID3D11Device* device, const D3D11_BUFFER_DESC& desc, const D3D11_SUBRESOURCE_DATA* subresourceData, ID3D11Buffer*& outBuffer)
	{
		HRESULT hr = device->CreateBuffer(&desc, subresourceData, &outBuffer);
		DX_CHECK_RESULT(hr);
	}

	void CreateShaderResourceView(ID3D11Device* device, ID3D11Resource* resource, const D3D11_SHADER_RESOURCE_VIEW_DESC& desc, ID3D11ShaderResourceView*& outView)
	{
		HRESULT hr = device->CreateShaderResourceView(resource, &desc, &outView);
		if (FAILED(hr))
		{
			auto renderer = DirectXRenderer::Get();
			renderer->HandleError(hr);
		}
	}

	void CreateRenderTargetView(ID3D11Device* device, ID3D11Resource* resource, const D3D11_RENDER_TARGET_VIEW_DESC& desc, ID3D11RenderTargetView*& outView)
	{
		HRESULT hr = device->CreateRenderTargetView(resource, &desc, &outView);
		if (FAILED(hr))
		{
			auto renderer = DirectXRenderer::Get();
			renderer->HandleError(hr);
		}
	}

	void CreateDepthStencilView(ID3D11Device* device, ID3D11Resource* resource, const D3D11_DEPTH_STENCIL_VIEW_DESC& desc, ID3D11DepthStencilView*& outView)
	{
		HRESULT hr = device->CreateDepthStencilView(resource, &desc, &outView);
		if (FAILED(hr))
		{
			auto renderer = DirectXRenderer::Get();
			renderer->HandleError(hr);
		}
	}

	void CreateUnorderedAccessView(ID3D11Device* device, ID3D11Resource* resource, const D3D11_UNORDERED_ACCESS_VIEW_DESC& desc, ID3D11UnorderedAccessView*& outView)
	{
		HRESULT hr = device->CreateUnorderedAccessView(resource, &desc, &outView);
		if (FAILED(hr))
		{
			auto renderer = DirectXRenderer::Get();
			renderer->HandleError(hr);
		}
	}

	void CreateBlendState(ID3D11Device* device, D3D11_BLEND_DESC& desc, ID3D11BlendState*& outState)
	{
		HRESULT hr = device->CreateBlendState(&desc, &outState);
		if (FAILED(hr))
		{
			auto renderer = DirectXRenderer::Get();
			renderer->HandleError(hr);
		}
	}

	void CreateTexture2D(ID3D11Device* device, const D3D11_TEXTURE2D_DESC& desc, const D3D11_SUBRESOURCE_DATA* subresourceData, ID3D11Texture2D*& outTexture)
	{
		HRESULT hr = device->CreateTexture2D(&desc, subresourceData, &outTexture);
		if (FAILED(hr))
		{
			auto renderer = DirectXRenderer::Get();
			renderer->HandleError(hr);
		}
	}

	void CreateSamplerState(ID3D11Device* device, const D3D11_SAMPLER_DESC& desc, ID3D11SamplerState*& outSampler)
	{
		HRESULT hr = device->CreateSamplerState(&desc, &outSampler);
		if (FAILED(hr))
		{
			auto renderer = DirectXRenderer::Get();
			renderer->HandleError(hr);
		}
	}

	void CreateComputeShader(ID3D11Device* device, Buffer binary, ID3D11ClassLinkage* classLinkage, ID3D11ComputeShader*& outComputeShader)
	{
		HRESULT hr = device->CreateComputeShader(binary.Data, binary.Size, classLinkage, &outComputeShader);
		if (FAILED(hr))
		{
			auto renderer = DirectXRenderer::Get();
			renderer->HandleError(hr);
		}
	}

	void CreateQuery(ID3D11Device* device, D3D11_QUERY queryType, ID3D11Query*& outQuery)
	{
		D3D11_QUERY_DESC desc;
		desc.Query = queryType;
		desc.MiscFlags = 0;
		CreateQuery(device, desc, outQuery);
	}

	void CreateQuery(ID3D11Device* device, D3D11_QUERY_DESC desc, ID3D11Query*& outQuery)
	{
		HRESULT hResult = device->CreateQuery(&desc, &outQuery);
		DX_CHECK_RESULT(hResult);
	}

}
