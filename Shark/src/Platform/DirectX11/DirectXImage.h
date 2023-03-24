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
		DirectXImage2D(const ImageSpecification& specs, Buffer imageData);
		DirectXImage2D(const ImageSpecification& specs, Ref<Image2D> data);
		DirectXImage2D(Ref<DirectXSwapChain> swapchain, bool createView);
		virtual ~DirectXImage2D();

		virtual void Invalidate() override;
		virtual void RT_Invalidate() override;

		virtual void Release() override;
		virtual void RT_Release() override;

		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }

		void RT_Invalidate(Ref<DirectXSwapChain> swapchain, bool createView = false);

		virtual void SetImageData(Buffer buffer) override;
		virtual void SetImageData(Ref<Image2D> image) override;
		virtual void RT_SetImageData(Ref<Image2D> image) override;

		virtual Ref<Image2D> RT_GetStorageImage() override;
		virtual bool RT_ReadPixel(uint32_t x, uint32_t y, uint32_t& out_Pixel) override;

		virtual void SetInitalData(Buffer initalData) override;
		virtual void RT_SetInitalData(Buffer initalData) override;
		virtual void ReleaseInitalData() override;
		virtual void RT_ReleaseInitalData() override;
		virtual Buffer RT_GetInitalData() const override { return m_InitalData; }

		virtual RenderID GetResourceID() const override { return m_Resource; }
		virtual RenderID GetViewID() const override { return m_View; }
		virtual const ImageSpecification& GetSpecification() const override { return m_Specification; }
		virtual ImageSpecification& GetSpecificationMutable() override { return m_Specification; }

	public:
		ID3D11Texture2D* GetResourceNative() const { return m_Resource; }
		ID3D11ShaderResourceView* GetViewNative() const { return m_View; }

	private:
		void UpdateResource(Buffer imageData);
		void UpdateResource(Ref<DirectXImage2D> imageData);

		bool IsDepthImage() { return m_Specification.Format == ImageFormat::Depth32; }
		bool IsImageCompadible(const ImageSpecification& specs) const { return m_Specification.Width == specs.Width && m_Specification.Height == specs.Height && m_Specification.Format == specs.Format && m_Specification.MipLevels == specs.MipLevels; }
		bool IsImageCompadibleIgnoreMipLeves(const ImageSpecification& specs) const { return m_Specification.Width == specs.Width && m_Specification.Height == specs.Height && m_Specification.Format == specs.Format; }

	private:
		void RT_UpdateResource(Buffer imageData);
		void RT_UpdateResource(Ref<DirectXImage2D> imageData);
		void RT_CreateView();

	private:
		ImageSpecification m_Specification;
		Buffer m_InitalData;

		ID3D11Texture2D* m_Resource = nullptr;
		ID3D11ShaderResourceView* m_View = nullptr;

		friend class DirectXRenderer;
	};

}
