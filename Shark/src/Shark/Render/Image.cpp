#include "skpch.h"
#include "Image.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Memory.h"
#include "Shark/Render/Renderer.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	namespace utils {

		nvrhi::ResourceStates GetDefaultResourceState(nvrhi::Format format, ImageUsage usage)
		{
			switch (usage)
			{
				case ImageUsage::Texture: return nvrhi::ResourceStates::ShaderResource;
				case ImageUsage::Storage: return nvrhi::ResourceStates::UnorderedAccess;
				case ImageUsage::Attachment: return nvrhi::getFormatInfo(format).hasDepth ? nvrhi::ResourceStates::DepthWrite : nvrhi::ResourceStates::RenderTarget;
			}

			SK_CORE_ASSERT(false, "Unknown ImageUsage");
			return nvrhi::ResourceStates::Unknown;
		}

	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	////// Image2D Implementation /////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

#pragma region Image2D

	Image2D::Image2D()
	{

	}

	Image2D::Image2D(const ImageSpecification& specification)
		: m_Specification(specification)
	{
		RT_Invalidate();
	}

	void Image2D::Invalidate()
	{
		if (m_Specification.MipLevels == 0)
			m_Specification.MipLevels = ImageUtils::CalcMipLevels(m_Specification.Width, m_Specification.Height);

		Ref instance = this;
		Renderer::Submit([instance, state = RT_State{ m_Specification }]()
		{
			instance->InvalidateFromState(state);
		});
	}

	void Image2D::RT_Invalidate()
	{
		if (m_Specification.MipLevels == 0)
			m_Specification.MipLevels = ImageUtils::CalcMipLevels(m_Specification.Width, m_Specification.Height);

		InvalidateFromState(m_Specification);
	}

	void Image2D::Resize(uint32_t width, uint32_t height)
	{
		if (m_Specification.Width == width && m_Specification.Height == height)
			return;

		SK_CORE_VERIFY(m_Specification.MipLevels == 1);
		m_Specification.Width = width;
		m_Specification.Height = height;

		Ref instance = this;
		Renderer::Submit([instance, state = RT_State{ m_Specification }]()
		{
			instance->InvalidateFromState(state);
		});
	}

	void Image2D::Resize(uint32_t width, uint32_t height, uint32_t mipLevels)
	{
		if (m_Specification.Width == width && m_Specification.Height == height && m_Specification.MipLevels == mipLevels)
			return;

		if (mipLevels == 0)
			mipLevels = ImageUtils::CalcMipLevels(width, height);

		m_Specification.Width = width;
		m_Specification.Height = height;
		m_Specification.MipLevels = mipLevels;

		Ref instance = this;
		Renderer::Submit([instance, state = RT_State{ m_Specification }]()
		{
			instance->InvalidateFromState(state);
		});
	}

	void Image2D::Submit_UploadData(const Buffer buffer)
	{
		Ref instance = this;
		Renderer::Submit([instance, tempBuffer = Buffer::Copy(buffer)]() mutable
		{
			instance->RT_UploadData(tempBuffer);
			tempBuffer.Release();
		});
	}

	void Image2D::RT_UploadData(const Buffer buffer)
	{
		SK_PROFILE_FUNCTION();

		auto deviceManager = Application::Get().GetDeviceManager();
		deviceManager->ExecuteCommand([this, buffer](nvrhi::ICommandList* cmd)
		{
			cmd->writeTexture(m_ImageHandle, 0, 0, buffer.As<const void>(), m_Specification.Width * ImageUtils::GetFormatBPP(m_Specification.Format));
		});
	}

	void Image2D::InvalidateFromState(const RT_State& state)
	{
		auto textureDesc = nvrhi::TextureDesc()
			.setWidth(state.Width)
			.setHeight(state.Height)
			.setArraySize(state.Layers)
			.setMipLevels(state.MipLevels)
			.setFormat(ImageUtils::ConvertImageFormat(state.Format))
			.setDebugName(state.DebugName);
		
		textureDesc.enableAutomaticStateTracking(utils::GetDefaultResourceState(textureDesc.format, state.Usage));
		textureDesc.isRenderTarget = state.Usage == ImageUsage::Attachment;
		textureDesc.isUAV = state.Usage == ImageUsage::Storage;
		textureDesc.isTypeless = ImageUtils::IsDepthFormat(state.Format);

		if (state.IsCube)
			textureDesc.dimension = nvrhi::TextureDimension::TextureCube;
		else if (state.Layers > 1)
			textureDesc.dimension = nvrhi::TextureDimension::Texture2DArray;

		auto device = Renderer::GetGraphicsDevice();
		m_ImageHandle = device->createTexture(textureDesc);
		m_ViewInfo.Handle = m_ImageHandle;

		SK_CORE_TRACE_TAG("Renderer", "Image Invalidated from state. '{}' {} ({}:{})", m_ImageHandle->getDesc().debugName, fmt::ptr(m_ImageHandle.Get()), state.Width, state.Height);
	}

#pragma endregion

	///////////////////////////////////////////////////////////////////////////////////////////////
	////// Image View Implementation //////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

#pragma region StagingImage2D

	MappedImageMemory::MappedImageMemory(MappedImageMemory&& other)
	{
		Texture = other.Texture;
		Memory = other.Memory;
		RowPitch = other.RowPitch;
		other.Texture = nullptr;
		other.Memory = {};
		other.RowPitch = 0;
	}

	MappedImageMemory::~MappedImageMemory()
	{
		auto device = Renderer::GetGraphicsDevice();

		device->unmapStagingTexture(Texture);
	}

	StagingImage2D::StagingImage2D(const StagingImageSpecification& specification)
		: m_Specification(specification)
	{
		if (m_Specification.MipLevels == 0)
			m_Specification.MipLevels = ImageUtils::CalcMipLevels(m_Specification.Width, m_Specification.Height);

		InvalidateFromState(m_Specification);
	}

	StagingImage2D::StagingImage2D(Ref<Image2D> templateImage, nvrhi::CpuAccessMode cpuAccess)
	{
		const auto& specification = templateImage->GetSpecification();
		m_Specification.Width = specification.Width;
		m_Specification.Height = specification.Height;
		m_Specification.Format = specification.Format;
		m_Specification.Layers = specification.Layers;
		m_Specification.MipLevels = specification.MipLevels;
		m_Specification.CpuAccess = cpuAccess;
		m_Specification.DebugName = fmt::format("{}:{}", specification.DebugName, cpuAccess);
		InvalidateFromState(m_Specification);
	}

	StagingImage2D::~StagingImage2D()
	{
	}

	void StagingImage2D::Resize(uint32_t width, uint32_t height)
	{
		if (m_Specification.Width == width && m_Specification.Height == height)
			return;

		SK_CORE_VERIFY(m_Specification.MipLevels == 1);
		m_Specification.Width = width;
		m_Specification.Height = height;

		Ref instance = this;
		Renderer::Submit([instance, state = RT_State{ m_Specification }]()
		{
			instance->InvalidateFromState(state);
		});
	}

	void StagingImage2D::Resize(uint32_t width, uint32_t height, uint32_t mipLevels)
	{
		if (m_Specification.Width == width && m_Specification.Height == height && m_Specification.MipLevels == mipLevels)
			return;

		if (mipLevels == 0)
			mipLevels = ImageUtils::CalcMipLevels(width, height);

		m_Specification.Width = width;
		m_Specification.Height = height;
		m_Specification.MipLevels = mipLevels;

		Ref instance = this;
		Renderer::Submit([instance, state = RT_State{ m_Specification }]()
		{
			instance->InvalidateFromState(state);
		});
	}

	MappedImageMemory StagingImage2D::RT_OpenReadable()
	{
		auto device = Renderer::GetGraphicsDevice();

		nvrhi::TextureSlice slice;
		slice.mipLevel = 0;
		slice.arraySlice = 0;

		MappedImageMemory mapped;
		mapped.Texture = m_Handle;
		mapped.Memory.Data = device->mapStagingTexture(m_Handle, slice, nvrhi::CpuAccessMode::Read, &mapped.RowPitch);
		mapped.Memory.Size = mapped.RowPitch * m_Specification.Height;
		return mapped;
	}

	void StagingImage2D::RT_OpenReadableBuffer(Buffer& outMemory)
	{
		auto device = Renderer::GetGraphicsDevice();

		nvrhi::TextureSlice slice;
		slice.mipLevel = 0;
		slice.arraySlice = 0;

		uint64_t rowPitch;
		void* memory = device->mapStagingTexture(m_Handle, slice, nvrhi::CpuAccessMode::Read, &rowPitch);

		outMemory = { memory, m_Specification.Height * rowPitch };
	}

	void StagingImage2D::RT_CloseReadableBuffer()
	{
		auto device = Renderer::GetGraphicsDevice();

		device->unmapStagingTexture(m_Handle);
	}

	void StagingImage2D::RT_ReadPixel(uint32_t x, uint32_t y, Buffer outPixel)
	{
		const uint32_t pixelSize = GetPixelSize();
		SK_CORE_VERIFY(outPixel.Size == pixelSize);

		auto device = Renderer::GetGraphicsDevice();

		nvrhi::TextureSlice slice;
		slice.mipLevel = 0;
		slice.arraySlice = 0;

		uint64_t rowPitch;
		void* memory = device->mapStagingTexture(m_Handle, slice, nvrhi::CpuAccessMode::Read, &rowPitch);

		const uint64_t pixelPitch = rowPitch / pixelSize;
		Memory::Read(memory, (y * pixelPitch + x) * pixelSize, outPixel.Data, pixelSize);

		device->unmapStagingTexture(m_Handle);
	}

	uint32_t StagingImage2D::GetPixelSize() const
	{
		return ImageUtils::GetFormatBPP(m_Specification.Format);
	}

	void StagingImage2D::InvalidateFromState(const RT_State& state)
	{
		auto textureDesc = nvrhi::TextureDesc()
			.setWidth(state.Width)
			.setHeight(state.Height)
			.setArraySize(state.Layers)
			.setMipLevels(state.MipLevels)
			.setFormat(ImageUtils::ConvertImageFormat(state.Format))
			.setDebugName(state.DebugName);

		//textureDesc.enableAutomaticStateTracking();
		textureDesc.isTypeless = ImageUtils::IsDepthFormat(state.Format);

		if (state.IsCube)
			textureDesc.dimension = nvrhi::TextureDimension::TextureCube;
		else if (state.Layers > 1)
			textureDesc.dimension = nvrhi::TextureDimension::Texture2DArray;

		auto device = Renderer::GetGraphicsDevice();
		m_Handle = device->createStagingTexture(textureDesc, state.CpuAccess);

		SK_CORE_TRACE_TAG("Renderer", "StagingImage Invalidated from state. '{}' {} ({}:{})", m_Handle->getDesc().debugName, fmt::ptr(m_Handle.Get()), state.Width, state.Height);
	}

#pragma endregion

	///////////////////////////////////////////////////////////////////////////////////////////////
	////// Image View Implementation //////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

#pragma region ImageView

	ImageView::ImageView(Ref<Image2D> image, const ImageViewSpecification& specification, bool initOnRT)
		: m_Image(image), m_Specification(specification)
	{
		SK_CORE_VERIFY(image);

		ImageFormat format = specification.Format;
		if (format == ImageFormat::None)
			format = image->GetSpecification().Format;

		m_StorageSupported = image->GetSpecification().Usage == ImageUsage::Storage && ImageUtils::SupportsUAV(specification.Format);

		if (!initOnRT)
		{
			InvalidateFromState(image, specification);
			return;
		}

		Ref instance = this;
		Renderer::Submit([instance, image, specification]()
		{
			instance->InvalidateFromState(image, specification);
		});
	}

	ImageView::~ImageView()
	{
	}

	void ImageView::InvalidateFromState(Ref<Image2D> image, const ViewState& viewState)
	{
		m_ViewInfo.Handle = image->GetHandle();
		m_ViewInfo.Format = ImageUtils::ConvertImageFormat(viewState.Format);
		m_ViewInfo.Dimension = viewState.Dimension;
		m_ViewInfo.SubresourceSet.baseMipLevel = viewState.BaseMip;
		m_ViewInfo.SubresourceSet.numMipLevels = viewState.MipCount;
		m_ViewInfo.SubresourceSet.baseArraySlice = viewState.BaseLayer;
		m_ViewInfo.SubresourceSet.numArraySlices = viewState.LayerCount;

		SK_CORE_TRACE_TAG("Renderer", "ImageView invalidated from state.");
	}

#pragma endregion

	///////////////////////////////////////////////////////////////////////////////////////////////
	////// Image Utilities ////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	namespace ImageUtils {

		uint32_t CalcMipLevels(uint32_t width, uint32_t height)
		{
			return (uint32_t)glm::floor(glm::log2((float)glm::max(width, height))) + 1;
		}

		glm::uvec2 CalcMipSize(uint32_t fullWidth, uint32_t fullHeight, uint32_t mip)
		{
			return { fullWidth >> mip, fullHeight >> mip };
		}

		nvrhi::Format ConvertImageFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None:    return nvrhi::Format::UNKNOWN;
				case ImageFormat::RGBA:    return nvrhi::Format::RGBA8_UNORM;
				case ImageFormat::sRGBA:   return nvrhi::Format::SRGBA8_UNORM;
				case ImageFormat::RG16F:   return nvrhi::Format::RG16_FLOAT;
				case ImageFormat::RGBA16F: return nvrhi::Format::RGBA16_FLOAT;
				case ImageFormat::RGBA32F: return nvrhi::Format::RGBA32_FLOAT;
				case ImageFormat::RED32SI: return nvrhi::Format::R32_SINT;
				case ImageFormat::RED32UI: return nvrhi::Format::R32_UINT;
				case ImageFormat::Depth32: return nvrhi::Format::D32;
				case ImageFormat::Depth24UNormStencil8UINT: return nvrhi::Format::D24S8;
			}
			SK_CORE_VERIFY(false, "Unknown ImageFormat");
			return nvrhi::Format::UNKNOWN;
		}

		ImageFormat ConvertImageFormat(nvrhi::Format format)
		{
			switch (format)
			{
				case nvrhi::Format::UNKNOWN:      return ImageFormat::None;
				case nvrhi::Format::RGBA8_UNORM:  return ImageFormat::RGBA;
				case nvrhi::Format::SRGBA8_UNORM: return ImageFormat::sRGBA;
				case nvrhi::Format::RG16_FLOAT:   return ImageFormat::RG16F;
				case nvrhi::Format::RGBA16_FLOAT: return ImageFormat::RGBA16F;
				case nvrhi::Format::RGBA32_FLOAT: return ImageFormat::RGBA32F;
				case nvrhi::Format::R32_SINT:     return ImageFormat::RED32SI;
				case nvrhi::Format::R32_UINT:     return ImageFormat::RED32UI;
				case nvrhi::Format::D32:          return ImageFormat::Depth32;
				case nvrhi::Format::D24S8:        return ImageFormat::Depth24UNormStencil8UINT;
			}
			SK_CORE_VERIFY(false);
			return ImageFormat::None;
		}

		ImageFormat ConvertToWritableFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::sRGBA: return ImageFormat::RGBA;
			}

			SK_CORE_ASSERT(!ImageUtils::IsSRGB(format));
			return format;
		}

		nvrhi::Format ConvertToWritableFormat(nvrhi::Format format)
		{
			switch (format)
			{
				case nvrhi::Format::SRGBA8_UNORM: return nvrhi::Format::RGBA8_UNORM;
			}

			SK_CORE_ASSERT(!nvrhi::getFormatInfo(format).isSRGB);
			return format;
		}

		bool IsSRGB(ImageFormat format)
		{
			return nvrhi::getFormatInfo(ConvertImageFormat(format)).isSRGB;
		}

		bool IsDepthFormat(Shark::ImageFormat format)
		{
			return nvrhi::getFormatInfo(ConvertImageFormat(format)).kind == nvrhi::FormatKind::DepthStencil;
		}

		bool IsIntegerBased(ImageFormat format)
		{
			return nvrhi::getFormatInfo(ConvertImageFormat(format)).kind == nvrhi::FormatKind::Integer;
		}

		bool SupportsUAV(ImageFormat format)
		{
			return !IsSRGB(format);
		}

		uint32_t GetFormatBPP(ImageFormat format)
		{
			return nvrhi::getFormatInfo(ConvertImageFormat(format)).bytesPerBlock;
		}

	}

}
