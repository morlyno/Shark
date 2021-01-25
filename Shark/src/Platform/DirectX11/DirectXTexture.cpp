#include "skpch.h"
#include "DirectXTexture.h"

#include <stb_image.h>

#include "Shark/Render/RendererCommand.h"
#include "Platform/DirectX11/DirectXRendererAPI.h"

#define SK_API() static_cast<::Shark::DirectXRendererAPI&>(::Shark::RendererCommand::GetRendererAPI())
#define SK_CHECK(call) if(HRESULT hr = (call); hr != S_OK) { ::std::ostringstream oss; oss << ::std::hex << hr; SK_CORE_ASSERT(false, oss.str()); }

namespace Shark {

	DirectXTexture2D::DirectXTexture2D(const std::string& filepath)
	{
		auto device = SK_API().GetDevice();

		int width, height;
		stbi_uc* data = stbi_load(filepath.c_str(), &width, &height, nullptr, 4);
		SK_CORE_ASSERT(data, "Failed to load Imiage!");

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
		SK_CHECK(device->CreateTexture2D(&td, &srd, &texture));

		D3D11_SHADER_RESOURCE_VIEW_DESC srv;
		srv.Format = td.Format;
		srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = 1u;
		srv.Texture2D.MostDetailedMip = 0u;

		SK_CHECK(device->CreateShaderResourceView(texture, &srv, &m_Texture));
		texture->Release();

		// TODO: Find Better place for this
		D3D11_SAMPLER_DESC sd = {};
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

		device->CreateSamplerState(&sd, &m_Sampler);
	}

	DirectXTexture2D::DirectXTexture2D(uint32_t width, uint32_t height, uint32_t color)
	{
		auto device = SK_API().GetDevice();

		std::vector<uint32_t> data;
		data.resize(width * height, color);

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

		// TODO: Find Better place for this
		D3D11_SAMPLER_DESC sd = {};
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

		device->CreateSamplerState(&sd, &m_Sampler);
	}

	DirectXTexture2D::~DirectXTexture2D()
	{
		if (m_Texture) { m_Texture->Release(); m_Texture = 0; }
		if (m_Sampler) { m_Sampler->Release(); m_Sampler = 0; }
	}

	void DirectXTexture2D::Bind()
	{
		auto context = SK_API().GetContext();

		context->PSSetSamplers(0u, 1u, &m_Sampler);
		context->PSSetShaderResources(0u, 1u, &m_Texture);
	}

}
