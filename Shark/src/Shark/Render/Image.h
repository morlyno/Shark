#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"

#include <glm/glm.hpp>

namespace Shark {

	enum class ImageFormat : uint16_t
	{
		None = 0,
		RGBA8,
		R32_SINT,

		Depth32,
		Depth = Depth32
	};
	std::string ToString(ImageFormat format);

	enum class ImageType : uint16_t
	{
		Texture,
		Storage,
		FrameBuffer
	};

	struct ImageSpecification
	{
		ImageFormat Format = ImageFormat::RGBA8;
		uint32_t Width = 0, Height = 0;
		uint32_t MipLevels = 1; // 0 == MaxLeves

		ImageType Type = ImageType::Texture;
	};

	class Image : public RefCount
	{
	public:
		virtual ~Image() = default;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
	};

	class Image2D : public Image
	{
	public:
		virtual ~Image2D() = default;

		virtual bool IsValid() const = 0;

		virtual void Set(const ImageSpecification& specs, Buffer imageData) = 0;
		virtual void Set(const ImageSpecification& specs, Ref<Image2D> data) = 0;
		virtual void Set(const std::filesystem::path& filePath) = 0;

		virtual void ReloadFromDisc() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual bool CopyTo(Ref<Image2D> image) = 0;

		virtual bool ReadPixel(uint32_t x, uint32_t y, uint32_t& out_Pixel) = 0;

		virtual RenderID GetResourceID() const = 0;
		virtual RenderID GetViewID() const = 0;
		virtual const ImageSpecification& GetSpecification() const = 0;

		virtual const std::filesystem::path& GetFilePath() const = 0;
		virtual void SetFilePath(const std::filesystem::path& filePath) = 0;

	public:
		virtual void RT_CopyTo(Ref<Image2D> image) = 0;

	public:
		static Ref<Image2D> Create();
		static Ref<Image2D> Create(const ImageSpecification& specs);
		static Ref<Image2D> Create(const ImageSpecification& specs, Buffer imageData);
		static Ref<Image2D> Create(const ImageSpecification& specs, Ref<Image2D> data);
		static Ref<Image2D> Create(ImageFormat format, uint32_t width, uint32_t height, Buffer imageData);
		static Ref<Image2D> Create(const std::filesystem::path& filePath);
	};

}
