#include "skpch.h"
#include "Texture.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/Renderer.h"
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
		: m_Specification(specification), m_Image(Image2D::Create()), m_ImageData(Buffer::Copy(imageData))
	{
		RT_Invalidate();
	}

	Texture2D::Texture2D(const TextureSpecification& specification, const std::filesystem::path& filepath)
		: m_Specification(specification), m_Image(Image2D::Create()), m_Filepath(filepath)
	{
		m_ImageData = TextureImporter::ToBufferFromFile(filepath, m_Specification.Format, m_Specification.Width, m_Specification.Height);
		if (!m_ImageData)
		{
			m_ImageData = TextureImporter::ToBufferFromFile("Resources/Textures/ErrorTexture.png", m_Specification.Format, m_Specification.Width, m_Specification.Height);
			SK_CORE_VERIFY(m_ImageData.Data);

			SetFlag(AssetFlag::Fallback, true);
		}

		if (m_Specification.DebugName.empty())
			m_Specification.DebugName = filepath.string();

		RT_Invalidate();
	}

	Texture2D::~Texture2D()
	{
		m_Image = nullptr;
		Release();
		m_ImageData.Release();
	}

	void Texture2D::Release()
	{
		if (m_Image)
			m_Image->Release();

		m_Sampler = nullptr;
	}

	void Texture2D::Submit_Invalidate()
	{
		m_Specification.GenerateMips = false;

		ImageSpecification& specification = m_Image->GetSpecification();
		specification.Width = m_Specification.Width;
		specification.Height = m_Specification.Height;
		specification.Format = m_Specification.Format;
		specification.MipLevels = m_Specification.GenerateMips ? 0 : 1;
		specification.DebugName = m_Specification.DebugName;

		m_Image->Submit_Invalidate();

		if (m_ImageData)
		{
			Renderer::Submit([image = m_Image, buffer = m_ImageData]() mutable
			{
				image->RT_UploadData(buffer);
				buffer.Release();
			});
			m_ImageData = {};

			if (m_Specification.GenerateMips)
				Renderer::GenerateMips(m_Image);
		}

		auto samplerDesc = nvrhi::SamplerDesc()
			.setMaxAnisotropy(m_Specification.MaxAnisotropy)
			.setAllFilters(m_Specification.Filter == FilterMode::Linear)
			.setAllAddressModes(utils::ConvertAddressMode(m_Specification.Address));
		
		Ref instance = this;
		Renderer::Submit([instance, samplerDesc]()
		{
			auto device = Application::Get().GetDeviceManager()->GetDevice();
			instance->m_Sampler = device->createSampler(samplerDesc);

			auto& viewInfo = instance->m_Image->GetViewInfo();
			viewInfo.Sampler = instance->m_Sampler;
		});

	}

	void Texture2D::RT_Invalidate()
	{
		m_Specification.GenerateMips = false;

		ImageSpecification& specification = m_Image->GetSpecification();
		specification.Width = m_Specification.Width;
		specification.Height = m_Specification.Height;
		specification.Format = m_Specification.Format;
		specification.MipLevels = m_Specification.GenerateMips ? 0 : 1;
		specification.DebugName = m_Specification.DebugName;

		m_Image->RT_Invalidate();

		if (m_ImageData)
		{
			m_Image->RT_UploadData(m_ImageData);
			m_ImageData.Release();

			if (m_Specification.GenerateMips)
				Renderer::RT_GenerateMips(m_Image);
		}

		auto samplerDesc = nvrhi::SamplerDesc()
			.setMaxAnisotropy(m_Specification.MaxAnisotropy)
			.setAllFilters(m_Specification.Filter == FilterMode::Linear)
			.setAllAddressModes(utils::ConvertAddressMode(m_Specification.Address));

		auto device = Application::Get().GetDeviceManager()->GetDevice();
		m_Sampler = device->createSampler(samplerDesc);

		auto& viewInfo = m_Image->GetViewInfo();
		viewInfo.Sampler = m_Sampler;
	}

	/////////////////////////////////////////////////////////////////////////////////////
	//////////////////// TextureCube ////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////

	TextureCube::TextureCube(const TextureSpecification& specification, Buffer imageData)
		: m_Specification(specification), m_ImageData(Buffer::Copy(imageData)), m_Image(Image2D::Create())
	{
		RT_Invalidate();
	}

	TextureCube::~TextureCube()
	{
		m_Image = nullptr;
		Release();
		m_ImageData.Release();
	}

	void TextureCube::Release()
	{
		if (m_Image)
			m_Image->Release();

		m_Sampler = nullptr;
	}

	void TextureCube::RT_Invalidate()
	{
		ImageSpecification& specification = m_Image->GetSpecification();
		specification.Width = m_Specification.Width;
		specification.Height = m_Specification.Height;
		specification.Format = m_Specification.Format;
		specification.Layers = 6;
		specification.IsCube = true;
		specification.MipLevels = m_Specification.GenerateMips ? 0 : 1;
		specification.DebugName = m_Specification.DebugName;
		m_Image->RT_Invalidate();

		if (m_ImageData)
		{
			m_Image->RT_UploadData(m_ImageData);
			m_ImageData.Release();

			if (m_Specification.GenerateMips)
				Renderer::RT_GenerateMips(m_Image);
		}

		auto samplerDesc = nvrhi::SamplerDesc()
			.setMaxAnisotropy(m_Specification.MaxAnisotropy)
			.setAllFilters(m_Specification.Filter == FilterMode::Linear)
			.setAllAddressModes(utils::ConvertAddressMode(m_Specification.Address));

		auto device = Application::Get().GetDeviceManager()->GetDevice();
		m_Sampler = device->createSampler(samplerDesc);

		auto& viewInfo = m_Image->GetViewInfo();
		viewInfo.Sampler = m_Sampler;
	}

}