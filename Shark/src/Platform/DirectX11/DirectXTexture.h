#pragma once

#include "Shark/Render/Texture.h"
#include "Platform/DirectX11/DirectXImage.h"

#include <d3d11.h>

namespace Shark {

	class DirectXSamplerWrapper : public SamplerWrapper
	{
	public:
		DirectXSamplerWrapper() = default;
		DirectXSamplerWrapper(const SamplerSpecification& spec);
		~DirectXSamplerWrapper();

		virtual void Release() override;
		virtual void Invalidate() override;

		virtual SamplerSpecification& GetSpecification() override { return m_Specification; };
		virtual const SamplerSpecification& GetSpecification() const override { return m_Specification; };

		virtual RenderID GetSamplerID() const override { return m_Sampler; }
		ID3D11SamplerState*& GetSampler() { return m_Sampler; }
		const ID3D11SamplerState* GetSampler() const { return m_Sampler; }

	private:
		void CreateSampler();

	private:
		SamplerSpecification m_Specification;
		ID3D11SamplerState* m_Sampler = nullptr;
	};

	class DirectXTexture2D : public Texture2D
	{
	public:
		DirectXTexture2D();
		DirectXTexture2D(const TextureSpecification& specification, Buffer imageData);
		DirectXTexture2D(const TextureSpecification& specification, Ref<TextureSource> textureSource);
		DirectXTexture2D(ImageFormat format, uint32_t width, uint32_t height, Buffer imageData);
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
		virtual Ref<SamplerWrapper> GetSampler() const override { return m_Sampler; }
		virtual Ref<Image2D> GetImage() const override { return m_Image; }

		virtual TextureSpecification& GetSpecification() override { return m_Specification; }
		virtual const TextureSpecification& GetSpecification() const override { return m_Specification; }

		DirectXImageInfo& GetDirectXImageInfo() { return m_Image->GetDirectXImageInfo(); }
		const DirectXImageInfo& GetDirectXImageInfo() const { return m_Image->GetDirectXImageInfo(); }
		ID3D11SamplerState* GetDirectXSampler() const { return m_Sampler->GetSampler(); }

	private:
		void UploadImageData();
		void RT_UploadImageData();

		Buffer GetCPUUploadBufer() const;

	private:
		TextureSpecification m_Specification;
		Ref<TextureSource> m_TextureSource;
		Buffer m_ImageData;

		Ref<DirectXImage2D> m_Image;
		Ref<DirectXSamplerWrapper> m_Sampler;

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

		virtual Ref<Image2D> GetImage() const override { return m_Image; }
		virtual Ref<SamplerWrapper> GetSampler() const override { return m_Sampler; }

		const DirectXImageInfo& GetDirectXImageInfo() const { return m_Image.As<DirectXImage2D>()->GetDirectXImageInfo(); }
		ID3D11SamplerState* GetDirectXSampler() const { return m_Sampler.As<DirectXSamplerWrapper>()->GetSampler(); }

	private:
		TextureSpecification m_Specification;
		Buffer m_ImageData;

		Ref<Image2D> m_Image;
		Ref<SamplerWrapper> m_Sampler;
	};

}