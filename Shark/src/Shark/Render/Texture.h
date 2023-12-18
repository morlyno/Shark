#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Render/Image.h"

#include <glm/glm.hpp>

namespace Shark {

	class TextureSource : public Asset
	{
	public:
		TextureSource() = default;
		virtual ~TextureSource()
		{
			ImageData.Release();
		}

		static AssetType GetStaticType() { return AssetType::TextureSource; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		static Ref<TextureSource> Create() { return Ref<TextureSource>::Create(); }

		Buffer ImageData;
		ImageFormat Format = ImageFormat::None;
		uint32_t Width = 0, Height = 0;

		std::filesystem::path SourcePath;
	};


	enum class FilterMode : uint16_t
	{
		None = 0,
		Nearest,
		Linear
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

	struct TextureWrapModes
	{
		WrapMode U, V, W;
		TextureWrapModes() = default;
		TextureWrapModes(WrapMode a) : U(a), V(a), W(a) {}
	};

	struct SamplerSpecification
	{
		FilterMode Filter = FilterMode::Linear;
		WrapMode Wrap = WrapMode::Repeat;
		bool Anisotropy = false;
		uint32_t MaxAnisotropy = 0;
	};

	struct TextureSpecification
	{
		uint32_t Width = 0, Height = 0;
		ImageFormat Format = ImageFormat::RGBA8;
		bool GenerateMips = false;
		SamplerSpecification Sampler;

		std::string DebugName;
	};

	class SamplerWrapper;

	class Texture2D : public Asset
	{
	public:
		virtual ~Texture2D() = default;

		virtual void Invalidate() = 0;
		virtual bool Validate() const = 0;

		virtual void Release() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetImageData(Buffer imageData) = 0;

		virtual Ref<TextureSource> GetTextureSource() const = 0;
		virtual void SetTextureSource(Ref<TextureSource> textureSource) = 0;

		virtual Buffer GetBuffer() const = 0;

		virtual RenderID GetViewID() const = 0;
		virtual Ref<SamplerWrapper> GetSampler() const = 0;
		virtual Ref<Image2D> GetImage() const = 0;
		virtual const TextureSpecification& GetSpecification() const = 0;
		virtual TextureSpecification& GetSpecificationMutable() = 0;

	public: // Asset Interface
		static AssetType GetStaticType() { return AssetType::Texture; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	public:
		static Ref<Texture2D> Create();
		static Ref<Texture2D> Create(const TextureSpecification& specification, Buffer imageData);
		static Ref<Texture2D> Create(const TextureSpecification& specification, Ref<TextureSource> textureSource);
		static Ref<Texture2D> Create(const SamplerSpecification& specification, Ref<Image2D> image, bool sharedImage = true);
		static Ref<Texture2D> Create(ImageFormat format, uint32_t width, uint32_t height, Buffer imageData);
		static Ref<Texture2D> Create(Ref<TextureSource> textureSource);

		static Ref<Texture2D> LoadFromDisc(const std::filesystem::path& filepath, const TextureSpecification& samplerSpecification = {});
	};

	class SamplerWrapper : public RefCount
	{
	public:
		virtual RenderID GetSamplerID() const = 0;

		static Ref<SamplerWrapper> Create(const SamplerSpecification& spec);
	};

}