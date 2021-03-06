#include "skpch.h"
#include "DirectXTexture.h"

#include <stb_image.h>

#include "Shark/Render/RendererCommand.h"
#include "Platform/DirectX11/DirectXRendererAPI.h"

#define SK_API() static_cast<::Shark::DirectXRendererAPI&>(::Shark::RendererCommand::GetRendererAPI())
#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); hr != S_OK) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
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

	DirectXTexture2D::DirectXTexture2D(const SamplerSpecification & specs, const std::string& filepath)
	{
		auto device = SK_API().GetDevice();

		int width, height;
		stbi_uc* data = stbi_load(filepath.c_str(), &width, &height, nullptr, 4);
		SK_CORE_ASSERT(data, "Failed to load Imiage!");

		m_Width = width;
		m_Height = height;

		size_t bn = filepath.find_last_of("/\\") + 1;
		m_Name = filepath.substr(bn, std::string::npos);

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
		SK_CHECK(device->CreateTexture2D(&td, &srd, &texture));

		D3D11_SHADER_RESOURCE_VIEW_DESC srv;
		srv.Format = td.Format;
		srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = 1u;
		srv.Texture2D.MostDetailedMip = 0u;

		SK_CHECK(device->CreateShaderResourceView(texture, &srv, &m_Texture));
		texture->Release();

		D3D11_SAMPLER_DESC sd;
		memset(&sd, 0, sizeof(D3D11_SAMPLER_DESC));
		sd.Filter = FilterModeToDirectX(specs.MinMag, specs.Mipmap);
		sd.AddressU = TextureAddressModeToDirectX(specs.AddressU);
		sd.AddressV = TextureAddressModeToDirectX(specs.AddressV);
		sd.AddressW = TextureAddressModeToDirectX(specs.AddressW);
		memcpy(sd.BorderColor, &specs.BorderColor, sizeof(float) * 4);

		device->CreateSamplerState(&sd, &m_Sampler);
	}

	DirectXTexture2D::DirectXTexture2D(const SamplerSpecification& specs, uint32_t width, uint32_t height, uint32_t color, const std::string& name)
		: m_Name(name), m_Width(width), m_Height(height)
	{
		auto device = SK_API().GetDevice();

		std::vector<uint32_t> data;
		data.resize((uint64_t)width * height, color);

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
		SK_CHECK(device->CreateTexture2D(&td, &srd, &texture));

		D3D11_SHADER_RESOURCE_VIEW_DESC srv;
		srv.Format = td.Format;
		srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = 1u;
		srv.Texture2D.MostDetailedMip = 0u;

		SK_CHECK(device->CreateShaderResourceView(texture, &srv, &m_Texture));
		texture->Release();

		D3D11_SAMPLER_DESC sd;
		memset(&sd, 0, sizeof(D3D11_SAMPLER_DESC));
		sd.Filter = FilterModeToDirectX(specs.MinMag, specs.Mipmap);
		sd.AddressU = TextureAddressModeToDirectX(specs.AddressU);
		sd.AddressV = TextureAddressModeToDirectX(specs.AddressV);
		sd.AddressW = TextureAddressModeToDirectX(specs.AddressW);
		memcpy(sd.BorderColor, &specs.BorderColor, sizeof(float) * 4);

		SK_CHECK(device->CreateSamplerState(&sd, &m_Sampler));
	}

	DirectXTexture2D::~DirectXTexture2D()
	{
		if (m_Texture) { m_Texture->Release(); }
		if (m_Sampler) { m_Sampler->Release(); }
	}

	void DirectXTexture2D::SetData(void* data)
	{
		auto device = SK_API().GetDevice();
		auto context = SK_API().GetContext();

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
		hr = device->CreateTexture2D(&desc, &srd, &newTexture);
		SK_CORE_ASSERT(SUCCEEDED(hr));

		context->CopyResource(texture, newTexture);
		newTexture->Release();
		texture->Release();
		resource->Release();

	}

	void DirectXTexture2D::Bind()
	{
		auto context = SK_API().GetContext();

		context->PSSetSamplers(m_Slot, 1u, &m_Sampler);
		context->PSSetShaderResources(m_Slot, 1u, &m_Texture);
	}

	void DirectXTexture2D::Bind(uint32_t slot)
	{
		auto context = SK_API().GetContext();

		context->PSSetSamplers(slot, 1u, &m_Sampler);
		context->PSSetShaderResources(slot, 1u, &m_Texture);
	}

}
