#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/RendererResource.h"
#include "Shark/Render/Image.h"

#include <glm/glm.hpp>

namespace Shark {

	enum class FilterMode : uint16_t
	{
		None = 0,
		Nearest,
		Linear,
		Anisotropic
	};

	enum class WrapMode : uint16_t
	{
		None = 0,
		Repeat,
		Clamp,
		Mirror
	};

	std::string ToString(FilterMode filterMode);
	std::string ToString(WrapMode wrapMode);

	std::string_view ToStringView(FilterMode filterMode);
	std::string_view ToStringView(WrapMode wrapMode);

	FilterMode StringToFilterMode(std::string_view filterMode);
	WrapMode StringToWrapMode(std::string_view wrapMode);

	struct TextureSpecification
	{
		uint32_t Width = 0, Height = 0;
		ImageFormat Format = ImageFormat::RGBA8;
		bool GenerateMips = true;

		FilterMode Filter = FilterMode::Linear;
		WrapMode Wrap = WrapMode::Repeat;
		uint32_t MaxAnisotropy = 0;

		std::string DebugName;
	};

	class Texture2D : public RendererResource
	{
	public:
		virtual ~Texture2D() = default;

		virtual void Invalidate() = 0;
		virtual void RT_Invalidate() = 0;
		virtual void Release() = 0;

		virtual bool IsValid() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual float GetAspectRatio() const = 0;
		virtual float GetVerticalAspectRatio() const = 0;

		virtual void SetImageData(Buffer imageData, bool copy = true) = 0;

		virtual Buffer& GetBuffer() = 0;
		virtual Buffer GetBuffer() const = 0;

		virtual RenderID GetViewID() const = 0;
		virtual Ref<Image2D> GetImage() const = 0;

		virtual TextureSpecification& GetSpecification() = 0;
		virtual const TextureSpecification& GetSpecification() const = 0;

		virtual const std::filesystem::path& GetFilepath() const = 0;
		virtual void SetFilepath(const std::filesystem::path& filepath) = 0;

		virtual AssetHandle GetSourceTextureHandle() const = 0;
		virtual void SetSourceTextureHandle(AssetHandle handle) = 0;

	public: // Asset Interface
		static AssetType GetStaticType() { return AssetType::Texture; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	public:
		static Ref<Texture2D> Create();
		static Ref<Texture2D> Create(const TextureSpecification& specification, Buffer imageData = Buffer());
		static Ref<Texture2D> Create(const TextureSpecification& specification, const std::filesystem::path& filepath);
	};

	class TextureCube : public RendererResource
	{
	public:
		virtual void Release() = 0;
		virtual void Invalidate() = 0;
		virtual void RT_Invalidate() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual uint32_t GetMipLevelCount() const = 0;

		virtual Ref<Image2D> GetImage() const = 0;

	public:
		static Ref<TextureCube> Create(const TextureSpecification& specification, Buffer imageData = {});
	};

}