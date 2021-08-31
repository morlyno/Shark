#include "skpch.h"
#include "DirectXTexture.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

#include <stb_image.h>

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	static D3D11_TEXTURE_ADDRESS_MODE TextureAddressModeToDirectX(AddressMode mode)
	{
		switch (mode)
		{
			case AddressMode::Wrap: return D3D11_TEXTURE_ADDRESS_WRAP;
			case AddressMode::Clamp: return D3D11_TEXTURE_ADDRESS_CLAMP;
			case AddressMode::Border: return D3D11_TEXTURE_ADDRESS_BORDER;
		}
		SK_CORE_ASSERT(false);
		return D3D11_TEXTURE_ADDRESS_WRAP;
	}

	static D3D11_FILTER FilterModeToDirectX(FilterMode minmag, FilterMode mipmap)
	{
		switch (minmag)
		{
		case FilterMode::Linera:
		{
			switch (mipmap)
			{
				case FilterMode::Linera: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
				case FilterMode::Point: return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			}
		}
		case FilterMode::Point:
		{
			switch (mipmap)
			{
				case FilterMode::Linera: return D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
				case FilterMode::Point: return D3D11_FILTER_MIN_MAG_MIP_POINT;
			}
		}
		}
		SK_CORE_ASSERT(false);
		return D3D11_FILTER_MIN_MAG_MIP_POINT;
	}

	DirectXTexture2D::DirectXTexture2D(ID3D11ShaderResourceView* texture, uint32_t width, uint32_t height, const SamplerProps& props)
		: m_Texture(texture), m_Width(width), m_Height(height)
	{
		CreateSampler(props);
	}

	DirectXTexture2D::DirectXTexture2D(const SamplerProps& props, const std::string& filepath)
		: m_FilePath(filepath)
	{
		int width, height;
		stbi_uc* data = stbi_load(filepath.c_str(), &width, &height, nullptr, 4);
		SK_CORE_ASSERT(data, fmt::format("Failed to load Imiage! {}", stbi_failure_reason()));

		m_Width = width;
		m_Height = height;

		CreateTexture(data);
		CreateSampler(props);
	}

	DirectXTexture2D::DirectXTexture2D(const SamplerProps& props, uint32_t width, uint32_t height, void* data)
		: m_FilePath(std::string{}), m_Width(width), m_Height(height)
	{
		CreateTexture(data);
		CreateSampler(props);
	}

	DirectXTexture2D::~DirectXTexture2D()
	{
		if (m_Texture) { m_Texture->Release(); }
		if (m_Sampler) { m_Sampler->Release(); }
	}

	void DirectXTexture2D::SetData(void* data)
	{
		// TODO: Check if this still works after the change for the CPU access!
		//       Function not available untill then further investigation
		SK_CORE_ASSERT(false);

		auto* ctx = DirectXRendererAPI::GetContext();
		auto* dev = DirectXRendererAPI::GetDevice();

		if (!m_Texture)
		{
			CreateTexture(data);
			return;
		}

		ID3D11Resource* resource;
		m_Texture->GetResource(&resource);
		SK_CORE_ASSERT(resource, "Failed to get Resource");
		
		ID3D11Texture2D* texture;
		HRESULT hr = resource->QueryInterface(&texture);
		SK_CORE_ASSERT(texture, "Failed to get Texture");
		SK_CORE_ASSERT(SUCCEEDED(hr));
		
		D3D11_TEXTURE2D_DESC desc;
		texture->GetDesc(&desc);
		
		D3D11_SUBRESOURCE_DATA srd;
		srd.pSysMem = data;
		srd.SysMemPitch = desc.Width * 4;
		
		ID3D11Texture2D* newTexture;
		SK_CHECK(dev->CreateTexture2D(&desc, &srd, &newTexture));

		// TODO: Test if this works
		//ctx->UpdateSubresource(texture, 0, nullptr, data, desc.Width * 4, 0)
		
		ctx->CopyResource(texture, newTexture);
		newTexture->Release();
		texture->Release();
		resource->Release();
	}

	void DirectXTexture2D::Bind(uint32_t slot)
	{
		auto* ctx = DirectXRendererAPI::GetContext();
		ctx->PSSetSamplers(slot, 1u, &m_Sampler);
		ctx->PSSetShaderResources(slot, 1u, &m_Texture);
	}

	void DirectXTexture2D::UnBind(uint32_t slot)
	{
		auto* ctx = DirectXRendererAPI::GetContext();
		ID3D11SamplerState* nullsplr = nullptr;
		ID3D11ShaderResourceView* nullsrv = nullptr;
		ctx->PSSetSamplers(slot, 1, &nullsplr);
		ctx->PSSetShaderResources(slot, 1, &nullsrv);
	}

	void DirectXTexture2D::CreateTexture(void* data)
	{
		auto* dev = DirectXRendererAPI::GetDevice();

		D3D11_TEXTURE2D_DESC td;
		td.Width = m_Width;
		td.Height = m_Height;
		td.MipLevels = 1u;
		td.ArraySize = 1u;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		td.SampleDesc.Count = 1u;
		td.SampleDesc.Quality = 0u;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		td.CPUAccessFlags = /*D3D11_CPU_ACCESS_WRITE*/0;
		td.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA srd;
		srd.pSysMem = data;
		srd.SysMemPitch = m_Width * 4;

		ID3D11Texture2D* texture;
		SK_CHECK(dev->CreateTexture2D(&td, data ? &srd : nullptr, &texture));

		D3D11_SHADER_RESOURCE_VIEW_DESC srv;
		srv.Format = td.Format;
		srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = 1u;
		srv.Texture2D.MostDetailedMip = 0u;

		SK_CHECK(dev->CreateShaderResourceView(texture, &srv, &m_Texture));
		texture->Release();

	}

	void DirectXTexture2D::CreateSampler(const SamplerProps& props)
	{
		auto* dev = DirectXRendererAPI::GetDevice();

		D3D11_SAMPLER_DESC sd;
		memset(&sd, 0, sizeof(D3D11_SAMPLER_DESC));
		sd.Filter = FilterModeToDirectX(props.MinMag, props.Mipmap);
		sd.AddressU = TextureAddressModeToDirectX(props.AddressU);
		sd.AddressV = TextureAddressModeToDirectX(props.AddressV);
		sd.AddressW = TextureAddressModeToDirectX(props.AddressW);
		memcpy(sd.BorderColor, &props.BorderColor, sizeof(float) * 4);

		SK_CHECK(dev->CreateSamplerState(&sd, &m_Sampler));

	}

}
