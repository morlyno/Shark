#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"

#include <glm/glm.hpp>

namespace Shark {

	class TextureSource;

	enum class ImageFormat : uint16_t
	{
		None = 0,
		RGBA8,
		RGBA16F,

		RGB32F,
		RGBA32F,

		R8,
		R16F,
		R32_SINT,

		Depth32,
		Depth = Depth32
	};
	std::string ToString(ImageFormat format);

	enum class ImageType : uint16_t
	{
		Texture,
		TextureCube,
		Storage,
		Atachment
	};

	std::string ToString(ImageType type);

	struct ImageSpecification
	{
		ImageFormat Format = ImageFormat::RGBA8;
		uint32_t Width = 0, Height = 0;
		uint32_t Layers = 1;
		uint32_t MipLevels = 1; // 0 == MaxLeves

		ImageType Type = ImageType::Texture;

		std::string DebugName;
	};

	class Image2D : public RefCount
	{
	public:
		virtual ~Image2D() = default;

		virtual void Release() = 0;
		virtual void Invalidate() = 0;
		virtual void RT_Invalidate() = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual bool IsValid(bool hasView = true) const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual float GetAspectRatio() const = 0;
		virtual float GetVerticalAspectRatio() const = 0;

		virtual void UploadImageData(Buffer buffer) = 0;
		virtual void RT_UploadImageData(Buffer buffer) = 0;

		virtual Ref<Image2D> RT_GetStorageImage() = 0;
		virtual bool RT_ReadPixel(uint32_t x, uint32_t y, uint32_t& out_Pixel) = 0;

		virtual RenderID GetViewID() const = 0;
		virtual ImageType GetType() const = 0;

		virtual ImageSpecification& GetSpecification() = 0;
		virtual const ImageSpecification& GetSpecification() const = 0;

	public:
		static Ref<Image2D> Create();
		static Ref<Image2D> Create(const ImageSpecification& specs);
	};
	
	class ImageView : public RefCount
	{
	public:
		virtual RenderID GetViewID() const = 0;

	public:
		static Ref<ImageView> Create(Ref<Image2D> image, uint32_t mipSlice);
	};

	namespace ImageUtils {

		uint32_t CalcMipLevels(uint32_t widht, uint32_t height);
		bool IsDepthFormat(ImageFormat format);
		bool IsIntegerBased(ImageFormat format);
		uint32_t GetFormatDataSize(ImageFormat format);

	}

}
