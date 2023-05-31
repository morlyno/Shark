#include "skpch.h"
#include "DirectXTexture.h"

#include "Shark/Core/Project.h"
#include "Platform/DirectX11/DirectXRenderer.h"

#include <stb_image.h>
#include "Shark/Render/Renderer.h"

namespace Shark {

	namespace utils {

		D3D11_FILTER_TYPE ConvertToDX11(FilterMode filterMode)
		{
			switch (filterMode)
			{
				case FilterMode::Nearest: return D3D11_FILTER_TYPE_POINT;
				case FilterMode::Linear:  return D3D11_FILTER_TYPE_LINEAR;
			}
			SK_CORE_ASSERT(false, "Unkown FilterMode");
			return (D3D11_FILTER_TYPE)0;
		}

		D3D11_TEXTURE_ADDRESS_MODE ConvertToDX11(WrapMode addressMode)
		{
			switch (addressMode)
			{
				case WrapMode::Repeat: return D3D11_TEXTURE_ADDRESS_WRAP;
				case WrapMode::Clamp:  return D3D11_TEXTURE_ADDRESS_CLAMP;
				case WrapMode::Mirror: return D3D11_TEXTURE_ADDRESS_MIRROR;
			}
			SK_CORE_ASSERT(false, "Unkown AddressMode");
			return (D3D11_TEXTURE_ADDRESS_MODE)0;
		}

		std::string GenerateSamplerName(const SamplerSpecification& specification)
		{
			if (specification.Anisotropy)
				return fmt::format("Sampler - Anisotropic {}", specification.MaxAnisotropy);
			return fmt::format("Sampler - {} {}", ToString(specification.Filter), ToString(specification.Wrap));
		}

	}

	DirectXTexture2D::DirectXTexture2D()
		: m_Image(Ref<DirectXImage2D>::Create())
	{
	}

	DirectXTexture2D::DirectXTexture2D(const TextureSpecification& specification, Buffer imageData)
		: m_Specification(specification), m_Image(Ref<DirectXImage2D>::Create()), m_ImageData(Buffer::Copy(imageData)), m_ImageDataOwned(true)
	{
		Invalidate();
	}

	DirectXTexture2D::DirectXTexture2D(const TextureSpecification& specification, Ref<TextureSource> textureSource)
		: m_Specification(specification), m_Image(Ref<DirectXImage2D>::Create())
	{
		SetTextureSource(textureSource);
		Invalidate();
	}

	DirectXTexture2D::DirectXTexture2D(const SamplerSpecification& specification, Ref<Image2D> image, bool sharedImage)
	{
		m_Specification.Width = image->GetWidth();
		m_Specification.Height = image->GetHeight();
		m_Specification.Format = image->GetSpecification().Format;
		m_Specification.Sampler = specification;

		Invalidate();
		
		if (sharedImage)
			m_Image = image.As<DirectXImage2D>();
		else
			m_Image = Ref<DirectXImage2D>::Create(image->GetSpecification(), image);

	}

	DirectXTexture2D::~DirectXTexture2D()
	{
		Release();
	}

	void DirectXTexture2D::Invalidate()
	{
		SK_CORE_VERIFY(m_Specification.Width != 0);
		SK_CORE_VERIFY(m_Specification.Height != 0);

#if SK_ENABLE_VERIFY
		if (m_TextureSource)
		{
			if (m_TextureSource->Width != m_Specification.Width || m_TextureSource->Height != m_Specification.Height || m_TextureSource->Format != m_Specification.Format)
			{
				SK_CORE_WARN_TAG(Tag::Renderer, "DirectXTexture2D::Invalidate - The Size or Format specified are different then the Size or Format from the TextureSource!\n"
					                            "\tThe Image will be initialized without any InitalData\n"
					                            "\tTexture: [{}, {}] - {} TextureSource: [{}, {}] - {}",
					                            m_Specification.Width, m_Specification.Height, ToString(m_Specification.Format), m_TextureSource->Width, m_TextureSource->Height, ToString(m_TextureSource->Format));
			}
		}
#endif

		Renderer::SubmitResourceFree([sampler = m_Sampler]()
		{
			if (sampler)
				sampler->Release();
		});

		m_Sampler = nullptr;

		if (m_Specification.DebugName.empty() && m_TextureSource)
			m_Specification.DebugName = m_TextureSource->SourcePath.string();

		// NOTE(moro): if the image is null it means the image gets set outside of Invalidate.
		//             for eaxample creating a texture with a shared image
		if (m_Image)
		{
			ImageSpecification& specification = m_Image->GetSpecificationMutable();
			specification.Format = m_Specification.Format;
			specification.Width = m_Specification.Width;
			specification.Height = m_Specification.Height;
			specification.MipLevels = m_Specification.GenerateMips ? 0 : 1;
			specification.Type = ImageType::Texture;
			specification.DebugName = m_Specification.DebugName;
			m_Image->Invalidate();
			UploadImageData();

			if (m_Specification.GenerateMips)
				Renderer::GenerateMips(m_Image);
		}

		Ref<DirectXTexture2D> instance = this;
		Renderer::Submit([instance]()
		{
			const auto& samplerSpec = instance->m_Specification.Sampler;
			D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT{});
			D3D11_FILTER_TYPE filter = utils::ConvertToDX11(samplerSpec.Filter);
			samplerDesc.Filter = samplerSpec.Anisotropy ?
				D3D11_FILTER_ANISOTROPIC :
				D3D11_ENCODE_BASIC_FILTER(filter, filter, D3D11_FILTER_TYPE_LINEAR, D3D11_FILTER_REDUCTION_TYPE_STANDARD);
			D3D11_TEXTURE_ADDRESS_MODE addressMode = utils::ConvertToDX11(samplerSpec.Wrap);
			samplerDesc.AddressU = addressMode;
			samplerDesc.AddressV = addressMode;
			samplerDesc.AddressW = addressMode;
			samplerDesc.MaxAnisotropy = samplerSpec.MaxAnisotropy;

			Ref<DirectXRenderer> renderer = DirectXRenderer::Get();
			ID3D11Device* device = renderer->GetDevice();
			DX11_VERIFY(device->CreateSamplerState(&samplerDesc, &instance->m_Sampler));

			std::string samplerName = utils::GenerateSamplerName(instance->m_Specification.Sampler);
			D3D_SET_OBJECT_NAME_A(instance->m_Sampler, samplerName.c_str());
		});
	}

	bool DirectXTexture2D::Validate() const
	{
		return m_Image->Validate() && m_Sampler;
	}

	void DirectXTexture2D::Release()
	{
		Renderer::SubmitResourceFree([sampler = m_Sampler]()
		{
			if (sampler)
				sampler->Release();
		});

		m_Sampler = nullptr;
		m_Image->Release();

		if (m_ImageData && m_ImageDataOwned)
		{
			m_ImageData.Release();
			m_ImageDataOwned = false;
		}

	}

	void DirectXTexture2D::SetImageData(Buffer imageData)
	{
		if (m_ImageData && m_ImageDataOwned)
			m_ImageData.Release();

		m_ImageData = Buffer::Copy(imageData);
		m_ImageDataOwned = true;
	}

	void DirectXTexture2D::SetTextureSource(Ref<TextureSource> textureSource)
	{
		if (m_ImageData && m_ImageDataOwned)
			m_ImageData.Release();

		m_TextureSource = textureSource;
		m_ImageData = textureSource->ImageData;
		m_ImageDataOwned = false;
		m_Specification.Width = textureSource->Width;
		m_Specification.Height = textureSource->Height;
		m_Specification.Format = textureSource->Format;
	}

	void DirectXTexture2D::UploadImageData()
	{
		SK_CORE_ASSERT(m_ImageData);
		Renderer::Submit([instance = Ref(this)]()
		{
			instance->m_Image->RT_UploadImageData(instance->m_ImageData);
		});
	}

#pragma region Texture Array

	DirectXTexture2DArray::DirectXTexture2DArray(uint32_t count, uint32_t startOffset)
		: m_Count(count), m_StartOffset(startOffset)
	{
		m_TextureArray.resize(count, nullptr);
		m_Views.resize(count, nullptr);
		m_Samplers.resize(count, nullptr);
	}

	void DirectXTexture2DArray::Set(uint32_t index, Ref<Texture2D> texture)
	{
		SetTexture(index, texture.As<DirectXTexture2D>());
	}

	Ref<Texture2D> DirectXTexture2DArray::Get(uint32_t index) const
	{
		SK_CORE_VERIFY(index < m_Count, "Index out of range");
		return m_TextureArray[index];
	}

	void DirectXTexture2DArray::RT_Set(uint32_t index, Ref<Texture2D> texture)
	{
		RT_SetTexture(index, texture.As<DirectXTexture2D>());
	}

	void DirectXTexture2DArray::SetTexture(uint32_t index, Ref<DirectXTexture2D> texture)
	{
		SK_CORE_VERIFY(index < m_Count, "Index out of range");

		Ref<DirectXTexture2DArray> instance = this;
		Renderer::Submit([instance, index, texture]()
		{
			instance->RT_SetTexture(index, texture);
		});
	}

	void DirectXTexture2DArray::RT_SetTexture(uint32_t index, Ref<DirectXTexture2D> texture)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		SK_CORE_VERIFY(index < m_Count, "Index out of range");

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

#pragma endregion

}
