#include "skpch.h"
#include "DirectXTexture.h"

#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"

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

	DirectXTexture2D::DirectXTexture2D(Ref<Image2D> image, const SamplerProps& props)
		: m_Image(image.As<DirectXImage2D>())
	{
		if (!m_Image->HasView())
			m_Image->CreateView();

		CreateSampler(props);
	}

	DirectXTexture2D::DirectXTexture2D(const std::filesystem::path& filepath, const SamplerProps& props)
		: m_FilePath(filepath)
	{
		ImageSpecification specs;
		specs.Usage = ImageUsageTexture;
		m_Image = Ref<DirectXImage2D>::Create(filepath, specs);

		if (!m_Image->HasView())
			m_Image->CreateView();

		CreateSampler(props);
	}

	DirectXTexture2D::DirectXTexture2D(uint32_t width, uint32_t height, void* data, const SamplerProps& props)
		: m_FilePath(std::string{})
	{
		ImageSpecification specs;
		specs.Width = width;
		specs.Height = height;
		specs.Usage = ImageUsageTexture;
		m_Image = Ref<DirectXImage2D>::Create(data, specs);

		if (!m_Image->HasView())
			m_Image->CreateView();

		CreateSampler(props);
	}

	DirectXTexture2D::~DirectXTexture2D()
	{
		if (m_Sampler) { m_Sampler->Release(); }
	}

	void DirectXTexture2D::Bind(ID3D11DeviceContext* ctx, uint32_t slot)
	{
		ctx->PSSetSamplers(slot, 1u, &m_Sampler);

		ID3D11ShaderResourceView* view = m_Image->GetViewNative();
		ctx->PSSetShaderResources(slot, 1u, &view);
	}

	void DirectXTexture2D::UnBind(ID3D11DeviceContext* ctx, uint32_t slot)
	{
		ID3D11SamplerState* nullsplr = nullptr;
		ID3D11ShaderResourceView* nullsrv = nullptr;
		ctx->PSSetSamplers(slot, 1, &nullsplr);
		ctx->PSSetShaderResources(slot, 1, &nullsrv);
	}

	void DirectXTexture2D::CreateSampler(const SamplerProps& props)
	{
		auto* dev = DirectXRenderer::GetDevice();

		D3D11_SAMPLER_DESC sd;
		memset(&sd, 0, sizeof(D3D11_SAMPLER_DESC));
		sd.Filter = FilterModeToDirectX(props.MinMag, props.Mipmap);
		sd.AddressU = TextureAddressModeToDirectX(props.AddressU);
		sd.AddressV = TextureAddressModeToDirectX(props.AddressV);
		sd.AddressW = TextureAddressModeToDirectX(props.AddressW);
		memcpy(sd.BorderColor, &props.BorderColor, sizeof(float) * 4);

		SK_CHECK(dev->CreateSamplerState(&sd, &m_Sampler));

	}

	DirectXTexture2DArray::DirectXTexture2DArray(uint32_t count)
	{
		m_TextureArray.resize(count);
	}

	void DirectXTexture2DArray::Resize(uint32_t newCount)
	{
		SK_CORE_VERIFY(newCount > m_TextureArray.size());
		if (newCount > m_TextureArray.size())
			m_TextureArray.resize(newCount);
	}

	Ref<Texture2D> DirectXTexture2DArray::Create(uint32_t index, Ref<Image2D> image, const SamplerProps& props)
	{
		Ref<DirectXTexture2D> texture = Ref<DirectXTexture2D>::Create(image, props);
		m_TextureArray[index] = texture;
		return texture;
	}

	Ref<Texture2D> DirectXTexture2DArray::Create(uint32_t index, const std::filesystem::path& filepath, const SamplerProps& props)
	{
		Ref<DirectXTexture2D> texture = Ref<DirectXTexture2D>::Create(filepath, props);
		m_TextureArray[index] = texture;
		return texture;
	}

	Ref<Texture2D> DirectXTexture2DArray::Create(uint32_t index, uint32_t width, uint32_t height, void* data, const SamplerProps& props)
	{
		Ref<DirectXTexture2D> texture = Ref<DirectXTexture2D>::Create(width, height, data, props);
		m_TextureArray[index] = texture;
		return texture;
	}

	void DirectXTexture2DArray::Set(uint32_t index, Ref<Texture2D> texture)
	{
		m_TextureArray[index] = texture.As<DirectXTexture2D>();
	}

	Ref<Texture2D> DirectXTexture2DArray::Get(uint32_t index) const
	{
		return m_TextureArray[index];
	}

}
