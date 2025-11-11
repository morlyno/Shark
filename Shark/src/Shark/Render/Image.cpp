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
		Ref instance = this;
		Renderer::Submit([instance]()
		{
			instance->m_ImageHandle = nullptr;
			instance->m_ViewInfo = {};
		});

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
		auto deviceManager = Application::Get().GetDeviceManager();
		auto commandList = deviceManager->GetCommandList(nvrhi::CommandQueue::Copy);

		commandList->open();
		commandList->writeTexture(m_ImageHandle, 0, 0, buffer.As<const void>(), m_Specification.Width * ImageUtils::GetFormatBPP(m_Specification.Format));
		commandList->close();

		deviceManager->ExecuteCommandListLocked(commandList);
	}

	void Image2D::InvalidateFromState(const RT_State& state)
	{
		// reading from images is only used for mouse picking currently
		// #Renderer #Investigate #TODO Staging Textures
		SK_CORE_VERIFY(state.Usage != ImageUsage::HostRead,
					   "HostRead (Stagin) Images are not implemented because it is not a normal texture in nvrhi."
					   "I am not sure how to handle this in Image at the moment.");

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

		auto device = Renderer::GetGraphicsDevice();
		m_ImageHandle = device->createTexture(textureDesc);
		m_ViewInfo.Handle = m_ImageHandle;

		SK_CORE_TRACE_TAG("Renderer", "Image Invalidated from state. '{}' {} ({}:{})", m_ImageHandle->getDesc().debugName, fmt::ptr(m_ImageHandle.Get()), state.Width, state.Height);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	////// Image View Implementation //////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

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
