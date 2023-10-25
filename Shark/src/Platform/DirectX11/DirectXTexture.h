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

		virtual RenderID GetSamplerID() const override { return m_Sampler; }

		void RT_SetSampler(ID3D11SamplerState* sampler);
		ID3D11SamplerState* RT_GetSampler() const { return m_Sampler; }

	private:
		void CreateSampler(const SamplerSpecification& spec);

	private:
		ID3D11SamplerState* m_Sampler;
	};

	class DirectXTexture2D : public Texture2D
	{
	public:
		DirectXTexture2D();
		DirectXTexture2D(const TextureSpecification& specification, Buffer imageData);
		DirectXTexture2D(const TextureSpecification& specification, Ref<TextureSource> textureSource);
		DirectXTexture2D(const SamplerSpecification& specification, Ref<Image2D> image, bool sharedImage = true);
		DirectXTexture2D(ImageFormat format, uint32_t width, uint32_t height, Buffer imageData);
		virtual ~DirectXTexture2D();

		virtual void Invalidate() override;
		virtual bool Validate() const override;

		virtual void Release() override;

		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }

		virtual void SetImageData(Buffer imageData) override;

		virtual Ref<TextureSource> GetTextureSource() const override { return m_TextureSource; }
		virtual void SetTextureSource(Ref<TextureSource> textureSource) override;

		virtual RenderID GetViewID() const override { return m_Image->GetViewID(); }
		virtual Ref<SamplerWrapper> GetSampler() const override { return m_SamplerWrapper; }
		virtual Ref<Image2D> GetImage() const override { return m_Image; }
		virtual const TextureSpecification& GetSpecification() const override { return m_Specification; }
		virtual TextureSpecification& GetSpecificationMutable() override { return m_Specification; }

	public:
		ID3D11SamplerState* GetSamplerNative() const { return m_Sampler; }
		ID3D11ShaderResourceView* GetViewNative() const { return m_Image->GetViewNative(); }

	private:
		void UploadImageData();

	private:
		TextureSpecification m_Specification;
		Ref<TextureSource> m_TextureSource;
		Buffer m_ImageData;
		bool m_ImageDataOwned = false;

		Ref<DirectXImage2D> m_Image;
		ID3D11SamplerState* m_Sampler = nullptr;
		Ref<DirectXSamplerWrapper> m_SamplerWrapper = Ref<DirectXSamplerWrapper>::Create();

		friend class DirectXRenderer;
	};

	class DirectXTexture2DArray : public Texture2DArray
	{
	public:
		DirectXTexture2DArray(uint32_t count, uint32_t startOffset = 0);
		virtual ~DirectXTexture2DArray() = default;

		virtual void Set(uint32_t index, Ref<Texture2D> texture) override;
		virtual Ref<Texture2D> Get(uint32_t index) const override;

		virtual uint32_t Count() const override { return m_Count; }

		virtual void RT_Set(uint32_t index, Ref<Texture2D> texture) override;

	private:
		void SetTexture(uint32_t index, Ref<DirectXTexture2D> texture);
		void RT_SetTexture(uint32_t index, Ref<DirectXTexture2D> texture);

	private:
		uint32_t m_Count;
		uint32_t m_StartOffset;
		std::vector<Ref<DirectXTexture2D>> m_TextureArray;
		std::vector<ID3D11ShaderResourceView*> m_Views;
		std::vector<ID3D11SamplerState*> m_Samplers;

		friend class DirectXRenderer;
	};

}