#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/RendererResource.h"
#include <nvrhi/nvrhi.h>

namespace Shark {

	//////////////////////////////////////////////////////////////////////////
	//// Viewable & ViewInfo 
	//////////////////////////////////////////////////////////////////////////

	struct ViewInfo
	{
		nvrhi::TextureHandle Handle;
		nvrhi::Format Format = nvrhi::Format::UNKNOWN;
		nvrhi::TextureDimension Dimension = nvrhi::TextureDimension::Unknown;
		nvrhi::TextureSubresourceSet SubresourceSet = nvrhi::AllSubresources;

		// Optional, only used by Texture and ImGui renderer
		nvrhi::SamplerHandle TextureSampler;

		bool operator==(const ViewInfo&) const = default;
	};

	class ViewableResource : public RendererResource
	{
	public:
		virtual const ViewInfo& GetViewInfo() const = 0;
		virtual bool HasSampler() const = 0;

	};

	//////////////////////////////////////////////////////////////////////////
	//// Image2D
	//////////////////////////////////////////////////////////////////////////

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

	class Image2D : public ViewableResource
	{
	public:
		static Ref<Image2D> Create() { return Ref<Image2D>::Create(); }
		static Ref<Image2D> Create(const ImageSpecification& specification) { return Ref<Image2D>::Create(specification); }

		void Release(); // #Renderer #Investigate Remove Release
		void Submit_Invalidate();
		void RT_Invalidate();
		void Resize(uint32_t width, uint32_t height);
		void Resize(uint32_t width, uint32_t height, uint32_t mipLevels);

		uint32_t GetWidth() const { return m_Specification.Width; }
		uint32_t GetHeight() const { return m_Specification.Height; }
		float GetAspectRatio() const { return (float)m_Specification.Width / m_Specification.Height; }
		float GetVerticalAspectRatio() const { return (float)m_Specification.Height / m_Specification.Width; }

		void Submit_UploadData(const Buffer buffer);
		void RT_UploadData(const Buffer buffer);
		//void RT_CopyToHostBuffer(Buffer& buffer);

		ImageSpecification& GetSpecification() { return m_Specification; }
		const ImageSpecification& GetSpecification() const { return m_Specification; }

		nvrhi::TextureHandle GetHandle() const { return m_ImageHandle; }
		virtual const ViewInfo& GetViewInfo() const override { return m_ViewInfo; }
		virtual bool HasSampler() const override { return false; }

	public:
		Image2D();
		Image2D(const ImageSpecification& specification);

	private:
		using RT_State = ImageSpecification;
		void InvalidateFromState(const RT_State& state);

	private:
		ImageSpecification m_Specification;

		// === RT ===
		nvrhi::TextureHandle m_ImageHandle;
		ViewInfo m_ViewInfo;
	};
	
	//////////////////////////////////////////////////////////////////////////
	//// ImageView
	//////////////////////////////////////////////////////////////////////////

	struct ImageViewSpecification
	{
		uint32_t BaseMip = 0;
		uint32_t MipCount = 1;
		uint32_t BaseLayer = 0;
		uint32_t LayerCount = 1;

		ImageFormat Format = ImageFormat::None;
		nvrhi::TextureDimension Dimension = nvrhi::TextureDimension::Unknown;

		static constexpr uint32_t AllMips = (uint32_t)-1;
		static constexpr uint32_t AllLayers = (uint32_t)-1;
	};

	class ImageView : public ViewableResource
	{
	public:
		static Ref<ImageView> Create(Ref<Image2D> image, const ImageViewSpecification& specification, bool initOnRT = false) { return Ref<ImageView>::Create(image, specification, initOnRT); }

		bool SupportsStorage() const { return m_StorageSupported; }
		const ImageViewSpecification& GetSpecification() const { return m_Specification; }
		virtual const ViewInfo& GetViewInfo() const override { return m_ViewInfo; }
		virtual bool HasSampler() const override { return false; }

	public:
		ImageView(Ref<Image2D> image, const ImageViewSpecification& specification, bool initOnRT);
		~ImageView();

	private:
		using ViewState = ImageViewSpecification;
		void InvalidateFromState(Ref<Image2D> image, const ViewState& viewState);

	private:
		Ref<Image2D> m_Image;
		ImageViewSpecification m_Specification;
		bool m_StorageSupported = false;

		// === RT ===
		ViewInfo m_ViewInfo;
	};

	//////////////////////////////////////////////////////////////////////////
	//// Image Utilities
	//////////////////////////////////////////////////////////////////////////
	
	namespace ImageUtils {

		uint32_t CalcMipLevels(uint32_t widht, uint32_t height);
		nvrhi::Format ConvertImageFormat(ImageFormat format);

		bool IsSRGB(ImageFormat format);
		bool IsDepthFormat(ImageFormat format);
		bool IsIntegerBased(ImageFormat format);
		bool SupportsUAV(ImageFormat format);
		uint32_t GetFormatBPP(ImageFormat format);

	}

}

namespace std {

	template<>
	struct hash<Shark::ViewInfo>
	{
		static_assert(sizeof(Shark::ViewInfo) == 40);
		size_t operator()(const Shark::ViewInfo& viewInfo) const
		{
			uint64_t hash = Shark::Hash::FNVBase;
			Shark::Hash::HashCombine(hash, Shark::StandartHash(viewInfo.Handle));
			Shark::Hash::HashCombine(hash, Shark::StandartHash(viewInfo.Format));
			Shark::Hash::HashCombine(hash, Shark::StandartHash(viewInfo.Dimension));
			Shark::Hash::HashCombine(hash, Shark::StandartHash(viewInfo.SubresourceSet));
			Shark::Hash::HashCombine(hash, Shark::StandartHash(viewInfo.TextureSampler));
			return hash;
		}
	};

}
