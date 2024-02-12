#include "skpch.h"
#include "DirectXTexture.h"

#include "Shark/Core/Project.h"
#include "Shark/Render/Renderer.h"

#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXRenderer.h"

namespace Shark {

	namespace utils {

		static D3D11_FILTER_TYPE ConvertToDX11(FilterMode filterMode)
		{
			switch (filterMode)
			{
				case FilterMode::Nearest: return D3D11_FILTER_TYPE_POINT;
				case FilterMode::Linear:  return D3D11_FILTER_TYPE_LINEAR;
			}
			SK_CORE_ASSERT(false, "Unkown FilterMode");
			return (D3D11_FILTER_TYPE)0;
		}

		static D3D11_TEXTURE_ADDRESS_MODE ConvertToDX11(WrapMode addressMode)
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

		static std::string GenerateSamplerName(const SamplerSpecification& specification)
		{
			if (specification.Filter == FilterMode::Anisotropic)
				return fmt::format("Sampler - Anisotropic {}", specification.MaxAnisotropy);
			return fmt::format("Sampler - {} {}", ToString(specification.Filter), ToString(specification.Wrap));
		}

		static D3D11_SAMPLER_DESC GetD3D11SamplerDesc(const SamplerSpecification& specification)
		{
			D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT{});
			D3D11_FILTER_TYPE filter = utils::ConvertToDX11(specification.Filter);
			samplerDesc.Filter = specification.Filter == FilterMode::Anisotropic ?
				D3D11_FILTER_ANISOTROPIC :
				D3D11_ENCODE_BASIC_FILTER(filter, filter, D3D11_FILTER_TYPE_LINEAR, D3D11_FILTER_REDUCTION_TYPE_STANDARD);
			D3D11_TEXTURE_ADDRESS_MODE addressMode = utils::ConvertToDX11(specification.Wrap);
			samplerDesc.AddressU = addressMode;
			samplerDesc.AddressV = addressMode;
			samplerDesc.AddressW = addressMode;
			samplerDesc.MaxAnisotropy = specification.MaxAnisotropy;
			return samplerDesc;
		}

	}

	DirectXTexture2D::DirectXTexture2D()
	{
	}

	DirectXTexture2D::DirectXTexture2D(const TextureSpecification& specification, Buffer imageData)
		: m_Specification(specification), m_ImageData(Buffer::Copy(imageData))
	{
		RT_Invalidate();
	}

	DirectXTexture2D::DirectXTexture2D(const TextureSpecification& specification, Ref<TextureSource> textureSource)
		: m_Specification(specification)
	{
		SetTextureSource(textureSource);
		RT_Invalidate();
	}

	DirectXTexture2D::DirectXTexture2D(ImageFormat format, uint32_t width, uint32_t height, Buffer imageData)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;
		m_Specification.Format = format;
		m_ImageData = Buffer::Copy(imageData);

		RT_Invalidate();
	}

	DirectXTexture2D::~DirectXTexture2D()
	{
		Release();
		m_ImageData.Release();
	}

	void DirectXTexture2D::Invalidate()
	{
		SK_CORE_VERIFY(m_Specification.Width != 0);
		SK_CORE_VERIFY(m_Specification.Height != 0);

		Release();

		if (m_Specification.DebugName.empty() && m_TextureSource)
			m_Specification.DebugName = m_TextureSource->SourcePath.string();

		ImageSpecification specification;
		specification.Format = m_Specification.Format;
		specification.Width = m_Specification.Width;
		specification.Height = m_Specification.Height;
		specification.MipLevels = m_Specification.GenerateMips ? 0 : 1;
		specification.Type = ImageType::Texture;
		specification.DebugName = m_Specification.DebugName;
		m_Image = Ref<DirectXImage2D>::Create(specification);

		Buffer uploadData = GetCPUUploadBufer();
		if (uploadData)
		{
			m_Image->UploadImageData(uploadData);

			if (m_Specification.GenerateMips)
				Renderer::GenerateMips(m_Image);
		}

		m_ImageData.Release();

		SamplerSpecification samplerSpec;
		samplerSpec.Filter = m_Specification.Filter;
		samplerSpec.Wrap = m_Specification.Wrap;
		samplerSpec.MaxAnisotropy = m_Specification.MaxAnisotropy;
		Ref<SamplerWrapper> sampler = SamplerWrapper::Create(samplerSpec);
		m_Sampler = sampler.As<DirectXSamplerWrapper>();
	}

	void DirectXTexture2D::RT_Invalidate()
	{
		SK_CORE_VERIFY(m_Specification.Width != 0);
		SK_CORE_VERIFY(m_Specification.Height != 0);

		Release();

		if (m_Specification.DebugName.empty() && m_TextureSource)
			m_Specification.DebugName = m_TextureSource->SourcePath.string();

		ImageSpecification specification;
		specification.Format = m_Specification.Format;
		specification.Width = m_Specification.Width;
		specification.Height = m_Specification.Height;
		specification.MipLevels = m_Specification.GenerateMips ? 0 : 1;
		specification.Type = ImageType::Texture;
		specification.DebugName = m_Specification.DebugName;
		m_Image = Ref<DirectXImage2D>::Create(specification);

		Buffer uploadData = GetCPUUploadBufer();
		if (uploadData)
		{
			m_Image->RT_UploadImageData(uploadData);

			if (m_Specification.GenerateMips)
				Renderer::RT_GenerateMips(m_Image);
		}

		m_ImageData.Release();

		SamplerSpecification samplerSpec;
		samplerSpec.Filter = m_Specification.Filter;
		samplerSpec.Wrap = m_Specification.Wrap;
		samplerSpec.MaxAnisotropy = m_Specification.MaxAnisotropy;
		Ref<SamplerWrapper> sampler = SamplerWrapper::Create(samplerSpec);
		m_Sampler = sampler.As<DirectXSamplerWrapper>();
	}

	void DirectXTexture2D::Release()
	{
		if (m_Image)
			m_Image->Release();

		if (m_Sampler)
			m_Sampler->Release();
	}

	void DirectXTexture2D::SetImageData(Buffer imageData, bool copy)
	{
		m_ImageData.Release();

		if (copy)
			m_ImageData = Buffer::Copy(imageData);
		else
			m_ImageData = imageData;
	}

	void DirectXTexture2D::SetTextureSource(Ref<TextureSource> textureSource)
	{
		m_TextureSource = textureSource;
		m_Specification.Width = textureSource->Width;
		m_Specification.Height = textureSource->Height;
		m_Specification.Format = textureSource->Format;
	}

	void DirectXTexture2D::UploadImageData()
	{
		Renderer::Submit([instance = Ref(this)]()
		{
			instance->RT_UploadImageData();
		});
	}

	void DirectXTexture2D::RT_UploadImageData()
	{
		Buffer uploadData = GetCPUUploadBufer();
		if (uploadData)
		{
			m_Image->RT_UploadImageData(uploadData);
		}

		m_ImageData.Release();
	}

	Buffer DirectXTexture2D::GetCPUUploadBufer() const
	{
		if (m_TextureSource)
			return m_TextureSource->ImageData;
		return m_ImageData;
	}

	DirectXSamplerWrapper::DirectXSamplerWrapper(const SamplerSpecification& spec)
		: m_Specification(spec)
	{
		CreateSampler();
	}

	DirectXSamplerWrapper::~DirectXSamplerWrapper()
	{
		Release();
	}

	void DirectXSamplerWrapper::Release()
	{
		Renderer::SubmitResourceFree([sampler = m_Sampler]()
		{
			DirectXAPI::ReleaseObject(sampler);
		});
		m_Sampler = nullptr;
	}

	void DirectXSamplerWrapper::Invalidate()
	{
		Ref<DirectXSamplerWrapper> instance = this;
		Renderer::Submit([instance]()
		{
			instance->CreateSampler();
		});
	}

	void DirectXSamplerWrapper::CreateSampler()
	{
		auto renderer = DirectXRenderer::Get();
		auto device = renderer->GetDevice();

		if (m_Specification.DebugName.empty())
			m_Specification.DebugName = utils::GenerateSamplerName(m_Specification);

		auto samplerDesc = utils::GetD3D11SamplerDesc(m_Specification);
		DirectXAPI::CreateSamplerState(device, samplerDesc, m_Sampler);
		DirectXAPI::SetDebugName(m_Sampler, m_Specification.DebugName);
	}

	DirectXTextureCube::DirectXTextureCube(const TextureSpecification& specification, Buffer imageData)
		: m_Specification(specification), m_ImageData(Buffer::Copy(imageData))
	{
		RT_Invalidate();
	}

	DirectXTextureCube::~DirectXTextureCube()
	{
		Release();
		m_ImageData.Release();
	}

	void DirectXTextureCube::Release()
	{
		if (m_Image)
			m_Image->Release();

		if (m_Sampler)
			m_Sampler->Release();
	}

	void DirectXTextureCube::Invalidate()
	{
		ImageSpecification specification;
		specification.Width = m_Specification.Width;
		specification.Height = m_Specification.Height;
		specification.Layers = 6;
		specification.Format = m_Specification.Format;
		specification.MipLevels = m_Specification.GenerateMips ? 0 : 1;
		specification.Type = ImageType::TextureCube;
		specification.DebugName = m_Specification.DebugName;
		m_Image = Image2D::Create(specification);

		if (m_ImageData)
		{
			m_Image->RT_UploadImageData(m_ImageData);

			if (m_Specification.GenerateMips)
				Renderer::RT_GenerateMips(m_Image);
		}

		m_ImageData.Release();

		SamplerSpecification samplerSpec;
		samplerSpec.Filter = m_Specification.Filter;
		samplerSpec.Wrap = m_Specification.Wrap;
		samplerSpec.MaxAnisotropy = m_Specification.MaxAnisotropy;
		m_Sampler = SamplerWrapper::Create(samplerSpec);
	}

	void DirectXTextureCube::RT_Invalidate()
	{
		ImageSpecification specification;
		specification.Width = m_Specification.Width;
		specification.Height = m_Specification.Height;
		specification.Layers = 6;
		specification.Format = m_Specification.Format;
		specification.MipLevels = m_Specification.GenerateMips ? 0 : 1;
		specification.Type = ImageType::TextureCube;
		specification.DebugName = m_Specification.DebugName;
		m_Image = Image2D::Create(specification);

		if (m_ImageData)
		{
			m_Image->RT_UploadImageData(m_ImageData);

			if (m_Specification.GenerateMips)
				Renderer::RT_GenerateMips(m_Image);
		}

		m_ImageData.Release();

		SamplerSpecification samplerSpec;
		samplerSpec.Filter = m_Specification.Filter;
		samplerSpec.Wrap = m_Specification.Wrap;
		samplerSpec.MaxAnisotropy = m_Specification.MaxAnisotropy;
		m_Sampler = SamplerWrapper::Create(samplerSpec);
	}

}
