#include "skpch.h"
#include "DirectXTexture.h"

#include "Shark/Core/Project.h"
#include "Platform/DirectX11/DirectXRenderer.h"

#include <stb_image.h>

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	namespace utils {

		D3D11_FILTER_TYPE FilterModeToD3D11(FilterMode filterMode)
		{
			switch (filterMode)
			{
				case FilterMode::Nearest: return D3D11_FILTER_TYPE_POINT;
				case FilterMode::Linear:  return D3D11_FILTER_TYPE_LINEAR;
			}
			SK_CORE_ASSERT(false, "Unkown FilterMode");
			return (D3D11_FILTER_TYPE)0;
		}

		D3D11_TEXTURE_ADDRESS_MODE AddressModeToD3D11(AddressMode addressMode)
		{
			switch (addressMode)
			{
				case AddressMode::Repeat: return D3D11_TEXTURE_ADDRESS_WRAP;
				case AddressMode::Clamp:  return D3D11_TEXTURE_ADDRESS_CLAMP;
				case AddressMode::Mirror: return D3D11_TEXTURE_ADDRESS_MIRROR;
				case AddressMode::Border: return D3D11_TEXTURE_ADDRESS_BORDER;
			}
			SK_CORE_ASSERT(false, "Unkown AddressMode");
			return (D3D11_TEXTURE_ADDRESS_MODE)0;
		}

		D3D11_FILTER MakeFilter(const SamplerSpecification& specs)
		{
			if (specs.Anisotropy)
				return D3D11_FILTER_ANISOTROPIC;
			
			return D3D11_ENCODE_BASIC_FILTER(
				utils::FilterModeToD3D11(specs.Min),
				utils::FilterModeToD3D11(specs.Mag),
				utils::FilterModeToD3D11(specs.Mip),
				D3D11_FILTER_REDUCTION_TYPE_STANDARD
			);
		}

	}

	DirectXTexture2D::DirectXTexture2D()
	{
	}

	DirectXTexture2D::DirectXTexture2D(const TextureSpecification& specs, void* data)
		: m_Specs(specs)
	{
		ImageSpecification imageSpecs;
		imageSpecs.Width = specs.Width;
		imageSpecs.Height = specs.Height;
		imageSpecs.Format = specs.Format;
		imageSpecs.MipLevels = specs.MipLevels;
		imageSpecs.Type = ImageType::Default;
		m_Image = Ref<DirectXImage2D>::Create(imageSpecs, data);

		CreateSampler();
	}

	DirectXTexture2D::DirectXTexture2D(ImageFormat format, uint32_t width, uint32_t height, void* data)
	{
		m_Specs.Format = format;
		m_Specs.Width = width;
		m_Specs.Height = height;

		ImageSpecification imageSpecs;
		imageSpecs.Width = m_Specs.Width;
		imageSpecs.Height = m_Specs.Height;
		imageSpecs.Format = m_Specs.Format;
		imageSpecs.Type = ImageType::Default;
		m_Image = Ref<DirectXImage2D>::Create(imageSpecs, data);

		CreateSampler();
	}

	DirectXTexture2D::DirectXTexture2D(const std::filesystem::path& filePath)
	{
		std::string narrorFilePath = filePath.string();
		int x, y, comp;
		stbi_uc* data = stbi_load(narrorFilePath.c_str(), &x, &y, &comp, STBI_rgb_alpha);
		if (!data)
		{
			SK_CORE_ERROR("Failed to load Image!");
			SK_CORE_WARN("Source: {}", Project::RelativeCopy(filePath));
			SK_CORE_WARN("Resource: {}", stbi_failure_reason());
			return;
		}

		ImageSpecification imageSpces;
		imageSpces.Format = ImageFormat::RGBA8;
		imageSpces.Width = x;
		imageSpces.Height = y;

		m_Image = Ref<DirectXImage2D>::Create(imageSpces, data);

		stbi_image_free(data);

		m_Specs.Format = ImageFormat::RGBA8;
		m_Specs.Width = x;
		m_Specs.Height = y;

		CreateSampler();
	}

	DirectXTexture2D::~DirectXTexture2D()
	{
		if (m_Sampler)
			m_Sampler->Release();
	}

	void DirectXTexture2D::Set(const TextureSpecification& specs, void* data)
	{
		m_Specs = specs;

		if (m_Sampler)
		{
			m_Sampler->Release();
			m_Sampler = nullptr;
			CreateSampler();

			ImageSpecification imageSpecs;
			imageSpecs.Width = m_Specs.Width;
			imageSpecs.Height = m_Specs.Height;
			imageSpecs.Format = m_Specs.Format;
			imageSpecs.MipLevels = m_Specs.MipLevels;
			imageSpecs.Type = ImageType::Default;
			m_Image->Set(imageSpecs, data);
			return;
		}

		ImageSpecification imageSpecs;
		imageSpecs.Width = m_Specs.Width;
		imageSpecs.Height = m_Specs.Height;
		imageSpecs.Format = m_Specs.Format;
		imageSpecs.MipLevels = m_Specs.MipLevels;
		imageSpecs.Type = ImageType::Default;
		m_Image = Ref<DirectXImage2D>::Create(imageSpecs, data);

		CreateSampler();
	}

	void DirectXTexture2D::CreateSampler()
	{
		SK_CORE_ASSERT(!m_Sampler, "Sampler already created");

		D3D11_SAMPLER_DESC desc{};
		desc.Filter = utils::MakeFilter(m_Specs.Sampler);
		desc.MaxAnisotropy = m_Specs.Sampler.MaxAnisotropy;

		desc.AddressU = utils::AddressModeToD3D11(m_Specs.Sampler.Address.U);
		desc.AddressV = utils::AddressModeToD3D11(m_Specs.Sampler.Address.V);
		desc.AddressW = utils::AddressModeToD3D11(m_Specs.Sampler.Address.W);

		desc.MipLODBias = m_Specs.Sampler.LODBias;
		desc.MinLOD = m_Specs.Sampler.MinLOD;
		desc.MaxLOD = m_Specs.Sampler.MaxLOD;

		for (uint32_t i = 0; i < 4; i++)
			desc.BorderColor[i] = m_Specs.Sampler.BorderColor[i];

		auto device = DirectXRenderer::GetDevice();
		SK_CHECK(device->CreateSamplerState(&desc, &m_Sampler));
	}


	DirectXTexture2DArray::DirectXTexture2DArray(uint32_t count, uint32_t startOffset)
		: m_Count(count), m_StartOffset(startOffset)
	{
		m_TextureArray.resize(count, nullptr);
		m_Views.resize(count, nullptr);
		m_Samplers.resize(count, nullptr);
	}

	Ref<Texture2D> DirectXTexture2DArray::Create(uint32_t index)
	{
		auto texture = Ref<DirectXTexture2D>::Create();
		SetTexture(index, texture);
		return texture;
	}

	Ref<Texture2D> DirectXTexture2DArray::Create(uint32_t index, const TextureSpecification& specs, void* data)
	{
		auto texture = Ref<DirectXTexture2D>::Create(specs, data);
		SetTexture(index, texture);
		return texture;
	}

	Ref<Texture2D> DirectXTexture2DArray::Create(uint32_t index, ImageFormat format, uint32_t width, uint32_t height, void* data)
	{
		auto texture = Ref<DirectXTexture2D>::Create(format, width, height, data);
		SetTexture(index, texture);
		return texture;
	}

	void DirectXTexture2DArray::Set(uint32_t index, Ref<Texture2D> texture)
	{
		SetTexture(index, texture.As<DirectXTexture2D>());
	}

	Ref<Texture2D> DirectXTexture2DArray::Get(uint32_t index) const
	{
		SK_CORE_ASSERT(index < m_Count, "Index out of range");
		return m_TextureArray[index];
	}

	void DirectXTexture2DArray::SetTexture(uint32_t index, Ref<DirectXTexture2D> texture)
	{
		SK_CORE_ASSERT(index < m_Count, "Index out of range");

		if (texture)
		{
			m_TextureArray[index] = texture;
			m_Views[index] = texture->GetViewNative();
			m_Samplers[index] = texture->GetSamplerNative();
		}
		else
		{
			m_TextureArray[index] = nullptr;
			m_Views[index] = nullptr;
			m_Samplers[index] = nullptr;
		}
	}

}
