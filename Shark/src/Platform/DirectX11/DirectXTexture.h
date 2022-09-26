#pragma once

#include "Shark/Render/Texture.h"
#include "Platform/DirectX11/DirectXImage.h"

#include <d3d11.h>

namespace Shark {

	class DirectXTexture2D : public Texture2D
	{
	public:
		DirectXTexture2D();
		DirectXTexture2D(const TextureSpecification& specs, void* data);
		DirectXTexture2D(ImageFormat format, uint32_t width, uint32_t height, void* data);
		DirectXTexture2D(const TextureSpecification& specs, Ref<Texture2D> data);
		DirectXTexture2D(const std::filesystem::path& filePath);
		virtual ~DirectXTexture2D();

		virtual void Set(const TextureSpecification& specs, void* data) override;
		virtual void SetSampler(const SamplerSpecification& specs) override;

		virtual void Set(const TextureSpecification& specs, Ref<Texture2D> data) override;

		virtual RenderID GetViewID() const override { return m_Image->GetViewID(); }
		virtual Ref<Image2D> GetImage() const override { return m_Image; }
		virtual const TextureSpecification& GetSpecification() const override { return m_Specs; }

		SK_DEPRECATED("Moved to Image2D")
		virtual const std::filesystem::path& GetFilePath() const override { return m_Image->GetFilePath(); }
		SK_DEPRECATED("Moved to Image2D")
		virtual void SetFilePath(const std::filesystem::path& filePath) override { m_Image->SetFilePath(filePath); }

		ID3D11SamplerState* GetSamplerNative() const { return m_Sampler; }
		ID3D11ShaderResourceView* GetViewNative() const { return m_Image->GetViewNative(); }

	private:
		void CreateSampler();

	private:
		TextureSpecification m_Specs;

		Ref<DirectXImage2D> m_Image;
		ID3D11SamplerState* m_Sampler = nullptr;;

		friend class DirectXRenderer;
	};

	class DirectXTexture2DArray : public Texture2DArray
	{
	public:
		DirectXTexture2DArray(uint32_t count, uint32_t startOffset = 0);
		virtual ~DirectXTexture2DArray() = default;

		virtual Ref<Texture2D> Create(uint32_t index) override;
		virtual Ref<Texture2D> Create(uint32_t index, const TextureSpecification& specs, void* data) override;
		virtual Ref<Texture2D> Create(uint32_t index, ImageFormat format, uint32_t width, uint32_t height, void* data) override;

		virtual void Set(uint32_t index, Ref<Texture2D> texture) override;
		virtual Ref<Texture2D> Get(uint32_t index) const override;

		virtual uint32_t Count() const override { return m_Count; }

	private:
		void SetTexture(uint32_t index, Ref<DirectXTexture2D> texture);

	private:
		uint32_t m_Count;
		uint32_t m_StartOffset;
		std::vector<Ref<DirectXTexture2D>> m_TextureArray;
		std::vector<ID3D11ShaderResourceView*> m_Views;
		std::vector<ID3D11SamplerState*> m_Samplers;

		friend class DirectXRenderer;
	};

}