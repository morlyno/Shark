#include "skpch.h"
#include "Texture.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Image.h"

#include "Shark/Serialization/Import/TextureImporter.h"

namespace Shark {

	namespace utils {

		static nvrhi::SamplerAddressMode ConvertAddressMode(AddressMode mode)
		{
			switch (mode)
			{
				case AddressMode::Repeat: return nvrhi::SamplerAddressMode::Repeat;
				case AddressMode::ClampToEdge: return nvrhi::SamplerAddressMode::ClampToEdge;
				case AddressMode::MirrorRepeat: return nvrhi::SamplerAddressMode::MirroredRepeat;
			}
			SK_CORE_ASSERT(false, "Unkown AddressMode");
			return nvrhi::SamplerAddressMode::Repeat;
		}

	}

	/////////////////////////////////////////////////////////////////////////////////////
	//////////////////// Texture2D //////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////

	Texture2D::Texture2D()
		: m_Image(Image2D::Create())
	{
	}

	Texture2D::Texture2D(const TextureSpecification& specification, const Buffer imageData)
		: m_Specification(specification), m_Image(Image2D::Create())
	{
		RT_Invalidate();

		if (imageData)
		{
			m_Image->RT_UploadData(imageData);
		}
	}

	Texture2D::Texture2D(const TextureSpecification& specification, const std::filesystem::path& filepath)
		: m_Specification(specification), m_Image(Image2D::Create()), m_Filepath(filepath)
	{
		auto imageData = TextureImporter::ToBufferFromFile(filepath, m_Specification.Format, m_Specification.Width, m_Specification.Height);
		if (!imageData)
		{
			imageData = TextureImporter::ToBufferFromFile("Resources/Textures/ErrorTexture.png", m_Specification.Format, m_Specification.Width, m_Specification.Height);
			SK_CORE_VERIFY(imageData);

			SetFlag(AssetFlag::Fallback, true);
		}

		if (m_Specification.DebugName.empty())
			m_Specification.DebugName = filepath.string();

		RT_Invalidate();
		m_Image->RT_UploadData(imageData);
	}

	Texture2D::~Texture2D()
	{
		m_Image = nullptr;
		m_ViewInfo.Handle = nullptr;
		m_ViewInfo.TextureSampler = nullptr;
	}

	void Texture2D::Invalidate()
	{
		ImageSpecification& specification = m_Image->GetSpecification();
		specification.Width = m_Specification.Width;
		specification.Height = m_Specification.Height;
		specification.Format = m_Specification.Format;
		specification.MipLevels = m_Specification.HasMips ? 0 : 1;
		specification.Usage = m_Specification.Storage ? ImageUsage::Storage : ImageUsage::Texture;
		specification.DebugName = m_Specification.DebugName;

		m_Image->Invalidate();

		Ref instance = this;
		Renderer::Submit([instance, image = m_Image, state = RT_State(m_Specification)]()
		{
			instance->InvalidateFromState(image, state);
		});
	}

	void Texture2D::RT_Invalidate()
	{
		ImageSpecification& specification = m_Image->GetSpecification();
		specification.Width = m_Specification.Width;
		specification.Height = m_Specification.Height;
		specification.Format = m_Specification.Format;
		specification.MipLevels = m_Specification.HasMips ? 0 : 1;
		specification.Usage = m_Specification.Storage ? ImageUsage::Storage : ImageUsage::Texture;
		specification.DebugName = m_Specification.DebugName;

		m_Image->RT_Invalidate();

		InvalidateFromState(m_Image, RT_State(m_Specification));
	}

	void Texture2D::Upload(BufferHandle imageData)
	{
		m_Image->Submit_UploadData(std::move(imageData));
	}

	void Texture2D::RT_Upload(const Buffer imageData)
	{
		m_Image->RT_UploadData(imageData);
	}

	uint32_t Texture2D::GetMipLevels() const
	{
		return m_Image->GetSpecification().MipLevels;
	}

	void Texture2D::InvalidateFromState(Ref<Image2D> image, const RT_State& state)
	{
		auto samplerDesc = nvrhi::SamplerDesc()
			.setMaxAnisotropy(state.MaxAnisotropy)
			.setAllFilters(state.Filter == FilterMode::Linear)
			.setAllAddressModes(utils::ConvertAddressMode(state.Address));

		auto device = Renderer::GetGraphicsDevice();
		m_ViewInfo.TextureSampler = device->createSampler(samplerDesc);

		m_ViewInfo.Handle = image->GetHandle();
		m_ViewInfo.SubresourceSet = nvrhi::AllSubresources;
		m_ViewInfo.Dimension = image->GetViewInfo().Dimension;
		m_ViewInfo.Format = image->GetViewInfo().Format;
	}

	uint32_t TextureCube::GetMipLevelCount() const
	{
		return m_Image->GetSpecification().MipLevels;
	}

	/////////////////////////////////////////////////////////////////////////////////////
	//////////////////// TextureCube ////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////

	TextureCube::TextureCube(const TextureSpecification& specification, Buffer imageData)
		: m_Specification(specification), m_Image(Image2D::Create())
	{
		ImageSpecification& imageSpec = m_Image->GetSpecification();
		imageSpec.Width = m_Specification.Width;
		imageSpec.Height = m_Specification.Height;
		imageSpec.Format = m_Specification.Format;
		imageSpec.Layers = 6;
		imageSpec.IsCube = true;
		imageSpec.MipLevels = m_Specification.HasMips ? 0 : 1;
		imageSpec.Usage = m_Specification.Storage ? ImageUsage::Storage : ImageUsage::Texture;
		imageSpec.DebugName = m_Specification.DebugName;
		m_Image->RT_Invalidate();

		if (imageData.Data)
		{
			m_Image->RT_UploadData(imageData);
		}

		auto samplerDesc = nvrhi::SamplerDesc()
			.setMaxAnisotropy(m_Specification.MaxAnisotropy)
			.setAllFilters(m_Specification.Filter == FilterMode::Linear)
			.setAllAddressModes(utils::ConvertAddressMode(m_Specification.Address));

		auto device = Renderer::GetGraphicsDevice();
		m_ViewInfo.TextureSampler = device->createSampler(samplerDesc);

		m_ViewInfo.Handle = m_Image->GetHandle();
		m_ViewInfo.SubresourceSet = nvrhi::AllSubresources;
		m_ViewInfo.Dimension = nvrhi::TextureDimension::TextureCube;
		m_ViewInfo.Format = m_Image->GetViewInfo().Format;
	}

	TextureCube::~TextureCube()
	{
		m_Image = nullptr;
	}

	/////////////////////////////////////////////////////////////////////////////////////
	//////////////////// Sampler ////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////

	Sampler::Sampler(const SamplerSpecification& specification)
		: m_Specification(specification)
	{
		auto samplerDesc = nvrhi::SamplerDesc()
			.setMaxAnisotropy(m_Specification.MaxAnisotropy)
			.setAllFilters(m_Specification.Filter == FilterMode::Linear)
			.setAllAddressModes(utils::ConvertAddressMode(m_Specification.Address));

		auto device = Renderer::GetGraphicsDevice();
		m_SamplerHandle = device->createSampler(samplerDesc);
	}

	Sampler::~Sampler()
	{
	}

}