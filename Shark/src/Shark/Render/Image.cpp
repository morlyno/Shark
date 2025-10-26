#include "skpch.h"
#include "Image.h"
#include "Shark/Core/Application.h"
#include "Renderer.h"

namespace Shark {

	///////////////////////////////////////////////////////////////////////////////////////////////
	////// Image2D Implementation /////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	Image2D::Image2D()
	{

	}

	Image2D::Image2D(const ImageSpecification& specification)
		: m_Specification(specification)
	{
		RT_Invalidate();
	}

	void Image2D::Release()
	{
		m_ImageHandle = nullptr;
		m_ViewInfo.ImageHandle = nullptr;
		m_ViewInfo.SubresourceSet = {};
		m_ViewInfo.Sampler = nullptr;
	}

	void Image2D::Submit_Invalidate()
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

		m_Specification.Width = width;
		m_Specification.Height = height;

		Submit_Invalidate();
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
		auto deviceManager = Application::Get().GetDeviceManager();
		auto commandList = deviceManager->GetCommandList(nvrhi::CommandQueue::Copy);

		commandList->open();
		commandList->writeTexture(m_ImageHandle, 0, 0, buffer.As<const void>(), m_Specification.Width * ImageUtils::GetFormatBPP(m_Specification.Format));
		commandList->close();

		deviceManager->ExecuteCommandListLocked(commandList);
	}

	void Image2D::InvalidateFromState(const RT_State& state)
	{
		// reading from images in only used for mouse picking currently
		SK_CORE_VERIFY(state.Usage != ImageUsage::Storage,
					   "Storage Images are not implemented because it is not a normal texture in nvrhi."
					   "\"Storage\" textures are StagingTextures in nvrhi, i am not sure how to handle this in Image.");

		auto textureDesc = nvrhi::TextureDesc()
			.setWidth(state.Width)
			.setHeight(state.Height)
			.setArraySize(state.Layers)
			.setMipLevels(state.MipLevels)
			.setFormat(ImageUtils::ConvertImageFormat(state.Format))
			.setDebugName(state.DebugName);

		textureDesc.isRenderTarget = state.Usage == ImageUsage::Attachment;
		textureDesc.isUAV = state.Usage == ImageUsage::Storage;
		textureDesc.isTypeless = ImageUtils::IsDepthFormat(state.Format);

		if (state.IsCube)
			textureDesc.dimension = nvrhi::TextureDimension::TextureCube;
		else if (state.Layers > 1)
			textureDesc.dimension = nvrhi::TextureDimension::Texture2DArray;

		auto cpuAccess = nvrhi::CpuAccessMode::None;

		if (state.Usage == ImageUsage::Attachment)
		{
			textureDesc.keepInitialState = true;
			textureDesc.initialState = nvrhi::getFormatInfo(textureDesc.format).hasDepth ?
				nvrhi::ResourceStates::DepthWrite : nvrhi::ResourceStates::RenderTarget;
		}
		else if (state.Usage == ImageUsage::Texture)
		{
			textureDesc.keepInitialState = true;
			textureDesc.initialState = nvrhi::ResourceStates::ShaderResource;
		}

		auto device = Application::Get().GetDeviceManager()->GetDevice();
		m_ImageHandle = device->createTexture(textureDesc);

		m_ViewInfo.ImageHandle = m_ImageHandle;
		m_ViewInfo.SubresourceSet.baseMipLevel = 0;
		m_ViewInfo.SubresourceSet.numMipLevels = state.MipLevels;
		m_ViewInfo.SubresourceSet.baseArraySlice = 0;
		m_ViewInfo.SubresourceSet.numArraySlices = state.Layers;

		SK_CORE_TRACE_TAG("Renderer", "Image Invalidated from state. '{}' {} ({}:{})", m_ImageHandle->getDesc().debugName, fmt::ptr(m_ImageHandle.Get()), state.Width, state.Height);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	////// Image View Implementation //////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	ImageView::ImageView(Ref<Image2D> image, const ImageViewSpecification& specification)
		: m_Image(image), m_Specification(specification)
	{
		RT_Invalidate();
	}

	ImageView::~ImageView()
	{
	}

	void ImageView::RT_Invalidate()
	{
		m_ViewInfo.ImageHandle = m_Image->GetHandle();
		m_ViewInfo.SubresourceSet.baseMipLevel = m_Specification.MipSlice;
		m_ViewInfo.SubresourceSet.numMipLevels = 1;
		m_ViewInfo.SubresourceSet.baseArraySlice = 0;
		m_ViewInfo.SubresourceSet.numArraySlices = m_Image->GetSpecification().Layers;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	////// Image Utilities ////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	namespace ImageUtils {

		uint32_t CalcMipLevels(uint32_t widht, uint32_t height)
		{
			return (uint32_t)glm::floor(glm::log2((float)glm::max(widht, height))) + 1;
		}

		nvrhi::Format ConvertImageFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None: return nvrhi::Format::UNKNOWN;
				case ImageFormat::RGBA: return nvrhi::Format::RGBA8_UNORM;
				case ImageFormat::sRGBA: return nvrhi::Format::SRGBA8_UNORM;
				case ImageFormat::RG16F: return nvrhi::Format::RG16_FLOAT;
				case ImageFormat::RGBA16F: return nvrhi::Format::RGBA16_FLOAT;
				case ImageFormat::RGBA32F: return nvrhi::Format::RGBA32_FLOAT;
				case ImageFormat::RED32SI: return nvrhi::Format::R32_SINT;
				case ImageFormat::Depth32: return nvrhi::Format::D32;
				case ImageFormat::Depth24UNormStencil8UINT: return nvrhi::Format::D24S8;
			}
			SK_CORE_ASSERT(false, "Unknown ImageFormat");
			return nvrhi::Format::UNKNOWN;
		}


		bool IsDepthFormat(Shark::ImageFormat format)
		{
			return nvrhi::getFormatInfo(ConvertImageFormat(format)).kind == nvrhi::FormatKind::DepthStencil;
		}

		bool IsIntegerBased(ImageFormat format)
		{
			return nvrhi::getFormatInfo(ConvertImageFormat(format)).kind == nvrhi::FormatKind::Integer;
		}

		uint32_t GetFormatBPP(ImageFormat format)
		{
			return nvrhi::getFormatInfo(ConvertImageFormat(format)).bytesPerBlock;
		}

	}

}
