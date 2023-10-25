#pragma once

#include "Shark/Core/Buffer.h"
#include "Shark/Render/Image.h"

#include <d3d11.h>

#undef UpdateResource

namespace Shark {

	namespace utils {

		DXGI_FORMAT ImageFormatToD3D11ForResource(ImageFormat format);
		DXGI_FORMAT ImageFormatToD3D11ForView(ImageFormat format);
		D3D11_USAGE UsageFromImageType(ImageType imageType);
		UINT CPUAccessFromType(ImageType imageType);
		uint32_t GetFormatDataSize(ImageFormat imageFormat);

	}

	class DirectXSwapChain;

	class DirectXImage2D : public Image2D
	{
	public:
		DirectXImage2D();
		DirectXImage2D(const ImageSpecification& specs);
		DirectXImage2D(const ImageSpecification& specs, Ref<Image2D> data);
		DirectXImage2D(const ImageSpecification& spec, ID3D11Texture2D* resource, bool createView);
		virtual ~DirectXImage2D();

		virtual void Invalidate() override;
		virtual void RT_Invalidate() override;

		virtual bool Validate(bool hasView = true) const override;

		virtual void Release() override;
		virtual void RT_Release() override;

		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }

		virtual void UploadImageData(Buffer buffer) override;
		virtual void RT_UploadImageData(Buffer buffer) override;
		virtual void UploadImageData(Ref<Image2D> image) override;
		virtual void RT_UploadImageData(Ref<Image2D> image) override;

		virtual Ref<Image2D> RT_GetStorageImage() override;
		virtual bool RT_ReadPixel(uint32_t x, uint32_t y, uint32_t& out_Pixel) override;

		virtual RenderID GetResourceID() const override { return m_Resource; }
		virtual RenderID GetViewID() const override { return m_View; }
		virtual const ImageSpecification& GetSpecification() const override { return m_Specification; }
		virtual ImageSpecification& GetSpecificationMutable() override { return m_Specification; }

	public:
		ID3D11Texture2D* GetResourceNative() const { return m_Resource; }
		ID3D11ShaderResourceView* GetViewNative() const { return m_View; }

		void SetResource(ID3D11Texture2D* resource) { m_Resource = resource; }

	private:
		void UpdateResource(Buffer imageData);
		void UpdateResource(Ref<DirectXImage2D> imageData);

		bool IsImageCompadible(const ImageSpecification& specs) const { return m_Specification.Width == specs.Width && m_Specification.Height == specs.Height && m_Specification.Format == specs.Format && m_Specification.MipLevels == specs.MipLevels; }
		bool IsImageCompadibleIgnoreMipLeves(const ImageSpecification& specs) const { return m_Specification.Width == specs.Width && m_Specification.Height == specs.Height && m_Specification.Format == specs.Format; }

	private:
		void RT_UpdateResource(Buffer imageData);
		void RT_UpdateResource(Ref<DirectXImage2D> imageData);
		void RT_CreateView();

	private:
		ImageSpecification m_Specification;

		ID3D11Texture2D* m_Resource = nullptr;
		ID3D11ShaderResourceView* m_View = nullptr;

		friend class DirectXRenderer;
	};

}
