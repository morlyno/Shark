#include "skpch.h"
#include "DirectXTexture.h"

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

	DirectXTexture2D::DirectXTexture2D(const SamplerSpecification & specs, const std::string& filepath, APIContext apicontext)
		: m_FilePath(filepath), m_APIContext(apicontext)
	{
		int width, height;
		stbi_uc* data = stbi_load(filepath.c_str(), &width, &height, nullptr, 4);
		SK_CORE_ASSERT(data, "Failed to load Imiage! " + (std::string)stbi_failure_reason());

		m_Width = width;
		m_Height = height;

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
		td.CPUAccessFlags = 0u;
		td.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA srd;
		srd.pSysMem = data;
		srd.SysMemPitch = m_Width * 4;

		ID3D11Texture2D* texture;
		SK_CHECK(m_APIContext.Device->CreateTexture2D(&td, &srd, &texture));

		D3D11_SHADER_RESOURCE_VIEW_DESC srv;
		srv.Format = td.Format;
		srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = 1u;
		srv.Texture2D.MostDetailedMip = 0u;

		SK_CHECK(m_APIContext.Device->CreateShaderResourceView(texture, &srv, &m_Texture));
		texture->Release();

		D3D11_SAMPLER_DESC sd;
		memset(&sd, 0, sizeof(D3D11_SAMPLER_DESC));
		sd.Filter = FilterModeToDirectX(specs.MinMag, specs.Mipmap);
		sd.AddressU = TextureAddressModeToDirectX(specs.AddressU);
		sd.AddressV = TextureAddressModeToDirectX(specs.AddressV);
		sd.AddressW = TextureAddressModeToDirectX(specs.AddressW);
		memcpy(sd.BorderColor, &specs.BorderColor, sizeof(float) * 4);

		SK_CHECK(m_APIContext.Device->CreateSamplerState(&sd, &m_Sampler));
	}

	DirectXTexture2D::DirectXTexture2D(const SamplerSpecification& specs, uint32_t width, uint32_t height, uint32_t flatcolor, APIContext apicontext)
		: m_FilePath(std::string{}), m_Width(width), m_Height(height), m_APIContext(apicontext)
	{
		std::vector<uint32_t> data;
		data.resize((uint64_t)width * height, flatcolor);

		D3D11_TEXTURE2D_DESC td;
		td.Width = m_Width;
		td.Height = m_Height;
		td.MipLevels = 1u;
		td.ArraySize = 1u;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		td.SampleDesc.Count = 1u;
		td.SampleDesc.Quality = 0u;
		td.Usage = D3D11_USAGE_DYNAMIC;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		td.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA srd;
		srd.pSysMem = data.data();
		srd.SysMemPitch = m_Width * 4;

		ID3D11Texture2D* texture;
		SK_CHECK(m_APIContext.Device->CreateTexture2D(&td, &srd, &texture));

		D3D11_SHADER_RESOURCE_VIEW_DESC srv;
		srv.Format = td.Format;
		srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = 1u;
		srv.Texture2D.MostDetailedMip = 0u;

		SK_CHECK(m_APIContext.Device->CreateShaderResourceView(texture, &srv, &m_Texture));
		texture->Release();

		D3D11_SAMPLER_DESC sd;
		memset(&sd, 0, sizeof(D3D11_SAMPLER_DESC));
		sd.Filter = FilterModeToDirectX(specs.MinMag, specs.Mipmap);
		sd.AddressU = TextureAddressModeToDirectX(specs.AddressU);
		sd.AddressV = TextureAddressModeToDirectX(specs.AddressV);
		sd.AddressW = TextureAddressModeToDirectX(specs.AddressW);
		memcpy(sd.BorderColor, &specs.BorderColor, sizeof(float) * 4);

		SK_CHECK(m_APIContext.Device->CreateSamplerState(&sd, &m_Sampler));
	}
	DirectXTexture2D::DirectXTexture2D(const SamplerSpecification& specs, uint32_t width, uint32_t height, const Buffer& data, APIContext apicontext)
		: m_FilePath(std::string{}), m_Width(width), m_Height(height), m_APIContext(apicontext)
	{
		D3D11_TEXTURE2D_DESC td;
		td.Width = m_Width;
		td.Height = m_Height;
		td.MipLevels = 1u;
		td.ArraySize = 1u;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		td.SampleDesc.Count = 1u;
		td.SampleDesc.Quality = 0u;
		td.Usage = D3D11_USAGE_DYNAMIC;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		td.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA srd;
		srd.pSysMem = data.Data();
		srd.SysMemPitch = m_Width * 4;

		ID3D11Texture2D* texture;
		SK_CHECK(m_APIContext.Device->CreateTexture2D(&td, &srd, &texture));

		D3D11_SHADER_RESOURCE_VIEW_DESC srv;
		srv.Format = td.Format;
		srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = 1u;
		srv.Texture2D.MostDetailedMip = 0u;

		SK_CHECK(m_APIContext.Device->CreateShaderResourceView(texture, &srv, &m_Texture));
		texture->Release();

		D3D11_SAMPLER_DESC sd;
		memset(&sd, 0, sizeof(D3D11_SAMPLER_DESC));
		sd.Filter = FilterModeToDirectX(specs.MinMag, specs.Mipmap);
		sd.AddressU = TextureAddressModeToDirectX(specs.AddressU);
		sd.AddressV = TextureAddressModeToDirectX(specs.AddressV);
		sd.AddressW = TextureAddressModeToDirectX(specs.AddressW);
		memcpy(sd.BorderColor, &specs.BorderColor, sizeof(float) * 4);

		SK_CHECK(m_APIContext.Device->CreateSamplerState(&sd, &m_Sampler));
	}

	DirectXTexture2D::~DirectXTexture2D()
	{
		if (m_Texture) { m_Texture->Release(); }
		if (m_Sampler) { m_Sampler->Release(); }
	}

	void DirectXTexture2D::SetData(const Buffer& data)
	{
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
		srd.pSysMem = data.Data();
		srd.SysMemPitch = desc.Width * 4;

		ID3D11Texture2D* newTexture;
		SK_CHECK(m_APIContext.Device->CreateTexture2D(&desc, &srd, &newTexture));

		m_APIContext.Context->CopyResource(texture, newTexture);
		newTexture->Release();
		texture->Release();
		resource->Release();

	}

	void DirectXTexture2D::Bind()
	{
		m_APIContext.Context->PSSetSamplers(m_Slot, 1u, &m_Sampler);
		m_APIContext.Context->PSSetShaderResources(m_Slot, 1u, &m_Texture);
	}

	void DirectXTexture2D::Bind(uint32_t slot)
	{
		m_APIContext.Context->PSSetSamplers(slot, 1u, &m_Sampler);
		m_APIContext.Context->PSSetShaderResources(slot, 1u, &m_Texture);
	}

}
