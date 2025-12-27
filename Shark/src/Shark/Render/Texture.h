#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/RendererResource.h"
#include "Shark/Render/Image.h"

namespace Shark {

	//////////////////////////////////////////////////////////////////////////
	//// Texture2D
	//////////////////////////////////////////////////////////////////////////

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
		bool Storage = false;

		float MaxAnisotropy = 1.0f;
		FilterMode Filter = FilterMode::Linear;
		AddressMode Address = AddressMode::Repeat;

		std::string DebugName;
	};

	class Texture2D : public ViewableResource
	{
	public:
		static Ref<Texture2D> Create() { return Ref<Texture2D>::Create(); }
		static Ref<Texture2D> Create(const TextureSpecification& specification, const Buffer imageData = Buffer()) { return Ref<Texture2D>::Create(specification, imageData); }
		static Ref<Texture2D> Create(const TextureSpecification& specification, const std::filesystem::path& filepath) { return Ref<Texture2D>::Create(specification, filepath); }

	public:
		void Invalidate();
		void RT_Invalidate();

		Ref<Image2D> GetImage() const { return m_Image; }
		virtual const ViewInfo& GetViewInfo() const override { return m_ViewInfo; }
		virtual bool HasSampler() const override { return true; }

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
		struct RT_State
		{
			float MaxAnisotropy = 1.0f;
			FilterMode Filter = FilterMode::Linear;
			AddressMode Address = AddressMode::Repeat;

			RT_State(const TextureSpecification& specification)
				: MaxAnisotropy(specification.MaxAnisotropy), Filter(specification.Filter), Address(specification.Address)
			{}
		};
		void InvalidateFromState(Ref<Image2D> image, const RT_State& state);

	private:
		TextureSpecification m_Specification;

		ViewInfo m_ViewInfo;
		Ref<Image2D> m_Image;

		Buffer m_ImageData;

		std::filesystem::path m_Filepath;
		AssetHandle m_SourceTextureHandle;
	};

	//////////////////////////////////////////////////////////////////////////
	//// TextureCube
	//////////////////////////////////////////////////////////////////////////

	class TextureCube : public ViewableResource
	{
	public:
		static Ref<TextureCube> Create(const TextureSpecification& specification, const Buffer imageData = {}) { return Ref<TextureCube>::Create(specification, imageData); }

	public:
		uint32_t GetWidth() const { return m_Specification.Width; }
		uint32_t GetHeight() const { return m_Specification.Height; }
		uint32_t GetMipLevelCount() const { return m_Image->GetSpecification().MipLevels; }

		Ref<Image2D> GetImage() const { return m_Image; }
		const TextureSpecification& GetSpecification() const { return m_Specification; }

		virtual const ViewInfo& GetViewInfo() const override { return m_ViewInfo; }
		virtual bool HasSampler() const override { return true; }

	public:
		TextureCube(const TextureSpecification& specification, const Buffer imageData);
		~TextureCube();

	private:
		TextureSpecification m_Specification;

		ViewInfo m_ViewInfo;
		Ref<Image2D> m_Image;

		Buffer m_ImageData;
	};

	//////////////////////////////////////////////////////////////////////////
	//// Sampler
	//////////////////////////////////////////////////////////////////////////

	struct SamplerSpecification
	{
		float MaxAnisotropy = 1.0f;
		FilterMode Filter = FilterMode::Linear;
		AddressMode Address = AddressMode::Repeat;
	};

	class Sampler : public RendererResource
	{
	public:
		static Ref<Sampler> Create(const SamplerSpecification& specification) { return Ref<Sampler>::Create(specification); }

	public:
		nvrhi::SamplerHandle GetHandle() const { return m_SamplerHandle; }
		const SamplerSpecification& GetSpecification() const { return m_Specification; }

	public:
		Sampler(const SamplerSpecification& specification);
		~Sampler();

	private:
		SamplerSpecification m_Specification;
		nvrhi::SamplerHandle m_SamplerHandle;

	};

}