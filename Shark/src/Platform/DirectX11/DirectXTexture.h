#pragma once

#include "Shark/Render/Texture.h"
#include "Platform/DirectX11/DirectXImage.h"

#include <d3d11.h>

namespace Shark {

	class DirectXTexture2D : public Texture2D
	{
	public:
		DirectXTexture2D();
		DirectXTexture2D(const TextureSpecification& specification, Buffer imageData);
		DirectXTexture2D(const TextureSpecification& specification, Ref<TextureSource> textureSource);
		virtual ~DirectXTexture2D();

		virtual void Invalidate() override;
		virtual void RT_Invalidate() override;
		virtual void Release() override;

		virtual bool IsValid() const override { return m_Image->IsValid(); }

		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }
		virtual float GetAspectRatio() const override { return (float)GetWidth() / (float)GetHeight(); }
		virtual float GetVerticalAspectRatio() const override { return (float)GetHeight() / (float)GetWidth(); }

		virtual void SetImageData(Buffer imageData, bool copy) override;

		virtual Buffer& GetBuffer() override { return m_ImageData; }
		virtual Buffer GetBuffer() const override { return m_ImageData; }

		virtual Ref<TextureSource> GetTextureSource() const override { return m_TextureSource; }
		virtual void SetTextureSource(Ref<TextureSource> textureSource) override;

		virtual RenderID GetViewID() const override { return m_Image->GetViewID(); }
		virtual Ref<Image2D> GetImage() const override { return m_Image; }

		virtual TextureSpecification& GetSpecification() override { return m_Specification; }
		virtual const TextureSpecification& GetSpecification() const override { return m_Specification; }

		DirectXImageInfo& GetDirectXImageInfo() { return m_Image->GetDirectXImageInfo(); }
		const DirectXImageInfo& GetDirectXImageInfo() const { return m_Image->GetDirectXImageInfo(); }

	private:
		void UploadImageData();
		void RT_UploadImageData();

		Buffer GetCPUUploadBufer() const;

	private:
		TextureSpecification m_Specification;
		Ref<TextureSource> m_TextureSource;
		Buffer m_ImageData;

		Ref<DirectXImage2D> m_Image;
		ID3D11SamplerState* m_Sampler = nullptr;

		friend class DirectXRenderer;
	};

	class DirectXTextureCube : public TextureCube
	{
	public:
		DirectXTextureCube(const TextureSpecification& specification, Buffer imageData);
		~DirectXTextureCube();

		virtual void Release() override;
		virtual void Invalidate() override;
		virtual void RT_Invalidate() override;

		virtual uint32_t GetWidth() const override { return m_Specification.Width; };
		virtual uint32_t GetHeight() const override { return m_Specification.Height; };

		virtual uint32_t GetMipLevelCount() const override { return m_Image->GetSpecification().MipLevels; }

		virtual Ref<Image2D> GetImage() const override { return m_Image; }

		const DirectXImageInfo& GetDirectXImageInfo() const { return m_Image.As<DirectXImage2D>()->GetDirectXImageInfo(); }
		ID3D11UnorderedAccessView*& GetUAV(uint32_t mipSlice) { return m_Image.As<DirectXImage2D>()->GetUAV(mipSlice); }

	private:
		TextureSpecification m_Specification;
		Buffer m_ImageData;

		Ref<Image2D> m_Image;
		ID3D11SamplerState* m_Sampler = nullptr;
	};

}