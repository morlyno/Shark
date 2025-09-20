#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/RendererResource.h"
#include <nvrhi/nvrhi.h>

namespace Shark {

	enum class ImageFormat : uint16_t
	{
		None = 0,
		RGBA,
		sRGBA,

		RG16F,
		RGBA16F,
		RGBA32F,

		RED32SI,

		Depth32,
		Depth24UNormStencil8UINT
	};

	enum class ImageType : uint16_t
	{
		Texture,
		TextureCube,
		Storage,
		Atachment
	};

	enum class ImageUsage
	{
		Texture,
		Attachment,
		Storage,
		HostRead
	};

	struct ImageSpecification
	{
		uint32_t Width = 0, Height = 0;
		ImageFormat Format = ImageFormat::RGBA;
		uint32_t Layers = 1;
		uint32_t MipLevels = 1; // 0 == MaxLeves

		ImageUsage Usage = ImageUsage::Texture;
		bool IsCube = false;

		std::string DebugName;
	};

	struct ViewInfo
	{
		nvrhi::TextureHandle ImageHandle;
		nvrhi::TextureSubresourceSet SubresourceSet;
		nvrhi::SamplerHandle Sampler;
	};

	class Image2D : public RendererResource
	{
	public:
		static Ref<Image2D> Create() { return Ref<Image2D>::Create(); }
		static Ref<Image2D> Create(const ImageSpecification& specification) { return Ref<Image2D>::Create(specification); }

		void Release();
		void Submit_Invalidate();
		void RT_Invalidate();
		void Resize(uint32_t width, uint32_t height);

		uint32_t GetWidth() const { return m_Specification.Width; }
		uint32_t GetHeight() const { return m_Specification.Height; }
		float GetAspectRatio() const { return (float)m_Specification.Width / m_Specification.Height; }
		float GetVerticalAspectRatio() const { return (float)m_Specification.Height / m_Specification.Width; }

		void Submit_UploadData(const Buffer buffer);
		void RT_UploadData(const Buffer buffer);
		//void RT_CopyToHostBuffer(Buffer& buffer);

		ImageSpecification& GetSpecification() { return m_Specification; }
		const ImageSpecification& GetSpecification() const { return m_Specification; }

		ViewInfo& GetViewInfo() { return m_ViewInfo; }
		const ViewInfo& GetViewInfo() const { return m_ViewInfo; }
		nvrhi::TextureHandle GetHandle() const { return m_ImageHandle; }

	public:
		Image2D();
		Image2D(const ImageSpecification& specification);

	private:
		using RT_State = ImageSpecification;
		void InvalidateFromState(const RT_State& state);

	private:
		ImageSpecification m_Specification;

		ViewInfo m_ViewInfo;
		nvrhi::TextureHandle m_ImageHandle;
	};
	
	struct ImageViewSpecification
	{
		uint32_t MipSlice = 0;
	};

	class ImageView : public RefCount
	{
	public:
		static Ref<ImageView> Create(Ref<Image2D> image, const ImageViewSpecification& specification) { return Ref<ImageView>::Create(image, specification); }

	public:
		void RT_Invalidate();

		Ref<Image2D> GetImage() const { return m_Image; }
		ImageViewSpecification& GetSpecification() { return m_Specification; }

		const ViewInfo& GetViewInfo() const { return m_ViewInfo; }

	public:
		ImageView(Ref<Image2D> image, const ImageViewSpecification& specification);
		~ImageView();

	private:
		ImageViewSpecification m_Specification;
		Ref<Image2D> m_Image;

		ViewInfo m_ViewInfo;
	};

	namespace ImageUtils {

		uint32_t CalcMipLevels(uint32_t widht, uint32_t height);
		nvrhi::Format ConvertImageFormat(ImageFormat format);

		bool IsDepthFormat(ImageFormat format);
		bool IsIntegerBased(ImageFormat format);
		uint32_t GetFormatBPP(ImageFormat format);

	}

}
