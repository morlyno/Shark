#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/RendererResource.h"
#include "Shark/Render/Image.h"

namespace Shark {

	enum class FilterMode : uint16_t
	{
		Nearest,
		Linear
	};

	enum class AddressMode : uint16_t
	{
		Repeat,
		ClampToEdge,
		MirrorRepeat
	};

	struct TextureSpecification
	{
		uint32_t Width = 0, Height = 0;
		ImageFormat Format = ImageFormat::RGBA;
		bool GenerateMips = true;

		float MaxAnisotropy = 1.0f;
		FilterMode Filter = FilterMode::Linear;
		AddressMode Address = AddressMode::Repeat;

		std::string DebugName;
	};

	class Texture2D : public RendererResource
	{
	public:
		static Ref<Texture2D> Create() { return Ref<Texture2D>::Create(); }
		static Ref<Texture2D> Create(const TextureSpecification& specification, const Buffer imageData = Buffer()) { return Ref<Texture2D>::Create(specification, imageData); }
		static Ref<Texture2D> Create(const TextureSpecification& specification, const std::filesystem::path& filepath) { return Ref<Texture2D>::Create(specification, filepath); }

	public:
		void Release();
		void Submit_Invalidate();
		void RT_Invalidate();

		Ref<Image2D> GetImage() const { return m_Image; }
		const ViewInfo& GetViewInfo() const { return m_Image->GetViewInfo(); }

		uint32_t GetWidth() const { return m_Specification.Width; }
		uint32_t GetHeight() const { return m_Specification.Height; }
		uint32_t GetMipLevels() const { return m_Image->GetSpecification().MipLevels; }
		float GetAspectRatio() const { return (float)GetWidth() / (float)GetHeight(); }
		float GetVerticalAspectRatio() const { return (float)GetHeight() / (float)GetWidth(); }

		Buffer& GetBuffer() { return m_ImageData; }
		Buffer GetBuffer() const { return m_ImageData; }

		TextureSpecification& GetSpecification() { return m_Specification; }
		const TextureSpecification& GetSpecification() const { return m_Specification; }

		const std::filesystem::path& GetFilepath() const { return m_Filepath; }
		void SetFilepath(const std::filesystem::path& filepath) { m_Filepath = filepath; }

		AssetHandle GetSourceTextureHandle() const { return m_SourceTextureHandle; }
		void SetSourceTextureHandle(AssetHandle handle) { m_SourceTextureHandle = handle; }

	public: // Asset Interface
		static AssetType GetStaticType() { return AssetType::Texture; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	public:
		Texture2D();
		Texture2D(const TextureSpecification& specification, const Buffer imageData = Buffer());
		Texture2D(const TextureSpecification& specification, const std::filesystem::path& filepath);
		~Texture2D();

	private:
		TextureSpecification m_Specification;
		Ref<Image2D> m_Image;
		nvrhi::SamplerHandle m_Sampler;

		Buffer m_ImageData;

		std::filesystem::path m_Filepath;
		AssetHandle m_SourceTextureHandle;
	};

	class TextureCube : public RendererResource
	{
	public:
		static Ref<TextureCube> Create(const TextureSpecification& specification, const Buffer imageData = {}) { return Ref<TextureCube>::Create(specification, imageData); }

	public:
		void Release();
		void RT_Invalidate();

		uint32_t GetWidth() const { return m_Specification.Width; }
		uint32_t GetHeight() const { return m_Specification.Height; }
		uint32_t GetMipLevelCount() const { return m_Image->GetSpecification().MipLevels; }

		Ref<Image2D> GetImage() const { return m_Image; }

	public:
		TextureCube(const TextureSpecification& specification, const Buffer imageData);
		~TextureCube();

	private:
		TextureSpecification m_Specification;
		Ref<Image2D> m_Image;
		nvrhi::SamplerHandle m_Sampler;

		Buffer m_ImageData;
	};

}