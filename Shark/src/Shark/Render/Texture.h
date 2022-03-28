#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/Image.h"
#include "Shark/Asset/Asset.h"

#include <glm/glm.hpp>

namespace Shark {

	enum class FilterMode { Nearest, Linear };
	enum class WrapMode { Repeat, Clamp, Mirror, Border };

	std::string EnumToString(FilterMode filterMode);
	std::string EnumToString(WrapMode wrapMode);

	struct TextureWrapModes
	{
		WrapMode U, V, W;
		TextureWrapModes() = default;
		TextureWrapModes(WrapMode a) : U(a), V(a), W(a) {}
	};

	struct SamplerSpecification
	{
		FilterMode Min = FilterMode::Linear;
		FilterMode Mag = FilterMode::Linear;
		FilterMode Mip = FilterMode::Linear;
		TextureWrapModes Wrap = WrapMode::Repeat;
		glm::vec4 BorderColor = glm::vec4(0);

		bool Anisotropy = false;
		uint32_t MaxAnisotropy = 0;

		float LODBias = 0.0f;
	};

	struct TextureSpecification
	{
		ImageFormat Format = ImageFormat::RGBA8;
		uint32_t Width = 0;
		uint32_t Height = 0;

		uint32_t MipLevels = 1;

		SamplerSpecification Sampler;
	};

	class Texture2D : public Asset
	{
	public:
		virtual ~Texture2D() = default;

		virtual void Set(const TextureSpecification& specs, void* data) = 0;
		virtual void SetSampler(const SamplerSpecification& specs) = 0;

		virtual void Set(const TextureSpecification& specs, Ref<Texture2D> data) = 0;

		virtual RenderID GetViewID() const = 0;
		virtual Ref<Image2D> GetImage() const = 0;
		virtual const TextureSpecification& GetSpecification() const = 0;

		virtual const std::filesystem::path& GetFilePath() const = 0;
		virtual void SetFilePath(const std::filesystem::path& filePath) = 0;

		static AssetType GetStaticType() { return AssetType::Texture; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	public:
		static Ref<Texture2D> Create();
		static Ref<Texture2D> Create(const TextureSpecification& specs, void* data);
		static Ref<Texture2D> Create(ImageFormat format, uint32_t width, uint32_t height, void* data);

		static Ref<Texture2D> Create(const TextureSpecification& specs, Ref<Texture2D> data);
		static Ref<Texture2D> Create(Ref<Texture2D> data) { return Create(data->GetSpecification(), data); }

		static Ref<Texture2D> Create(const std::filesystem::path& filePath);
	};

	class Texture2DArray : public RefCount
	{
	public:
		virtual ~Texture2DArray() = default;

		virtual Ref<Texture2D> Create(uint32_t index) = 0;
		virtual Ref<Texture2D> Create(uint32_t index, const TextureSpecification& specs, void* data) = 0;
		virtual Ref<Texture2D> Create(uint32_t index, ImageFormat format, uint32_t width, uint32_t height, void* data) = 0;

		virtual void Set(uint32_t index, Ref<Texture2D> texture) = 0;
		virtual Ref<Texture2D> Get(uint32_t index) const = 0;

		virtual uint32_t Count() const = 0;

	public:
		static Ref<Texture2DArray> Create(uint32_t count, uint32_t startOffset = 0);
	};

}