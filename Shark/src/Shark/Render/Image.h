#pragma once

#include "Shark/Core/Base.h"

#include <DirectXMath.h>

namespace Shark {

	enum class ImageFormat : uint16_t
	{
		None = 0,
		RGBA8,
		R32_SINT,

		Depth32,

		SwapChain,
		Depth = Depth32
	};
	std::string ImageFormatToString(ImageFormat format);

	enum class ImageType : uint16_t
	{
		Default,
		Immutable,
		Dynamic,
		Staging
	};
	std::string ImageTypeToString(ImageType usage);

	enum ImageUsage : uint16_t
	{
		ImageUsageNone = 0,
		ImageUsageTexture = SK_BIT(0),
		ImageUsageFrameBuffer = SK_BIT(1),
		ImageUsageDethStencil = SK_BIT(2)
	};
	std::string ImageUsageToString(uint32_t flags);

	struct ImageSpecification
	{
		uint32_t Width = 0, Height = 0;
		ImageFormat Format = ImageFormat::RGBA8;
		ImageType Type = ImageType::Default;
		uint32_t Usage = ImageUsageTexture;

		ImageSpecification() = default;
		ImageSpecification(ImageFormat format, uint32_t width, uint32_t height, ImageType type = ImageType::Default, uint32_t usage = ImageUsageTexture)
			: Format(format), Width(width), Height(height), Type(type), Usage(usage)
		{}
	};

	class Image2D : public RefCount
	{
	public:
		virtual ~Image2D() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void Resize(uint32_t widht, uint32_t height) = 0;

		virtual void SetData(void* data, uint32_t size) = 0;
		virtual void CopyTo(Ref<Image2D> dest) = 0;

		virtual uint32_t ReadPixel(uint32_t x, uint32_t y) = 0;

		virtual void CreateView() = 0;
		virtual bool HasView() const = 0;

		virtual RenderID GetRenderID() const = 0;
		virtual RenderID GetViewRenderID() const = 0;
		virtual const ImageSpecification& GetSpecification() const = 0;

		static Ref<Image2D> Create(const ImageSpecification& specs);
		static Ref<Image2D> Create(const ImageSpecification& specs, void* data);
		static Ref<Image2D> Create(const std::filesystem::path& filepath, const ImageSpecification& specs = ImageSpecification{});
	};

}
