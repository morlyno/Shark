#pragma once

#include "Shark/Render/Image.h"

#include <d3d11.h>

namespace Shark {

	namespace utils {

		DXGI_FORMAT ImageFormatToD3D11ForResource(ImageFormat format);
		DXGI_FORMAT ImageFormatToD3D11ForView(ImageFormat format);
		D3D11_USAGE UsageFromImageType(ImageType imageType);
		UINT CPUAccessFromType(ImageType imageType);

	}

	class DirectXImage2D : public Image2D
	{
	public:
		DirectXImage2D();
		DirectXImage2D(const ImageSpecification& specs, void* data);
		DirectXImage2D(ImageFormat format, uint32_t width, uint32_t height, void* data);
		virtual ~DirectXImage2D();

		virtual void Set(const ImageSpecification& specs, void* data) override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual bool CopyTo(Ref<Image2D> image) override;

		virtual bool ReadPixel(uint32_t x, uint32_t y, uint32_t& out_Pixel) override;

		virtual RenderID GetResourceID() const override { return m_Resource; }
		virtual RenderID GetViewID() const override { return m_View; }
		virtual const ImageSpecification& GetSpecification() const override { return m_Specs; }
		virtual uint32_t GetWidth() const override { return m_Specs.Width; }
		virtual uint32_t GetHeight() const override { return m_Specs.Width; }

		ID3D11Texture2D* GetResourceNative() const { return m_Resource; }
		ID3D11ShaderResourceView* GetViewNative() const { return m_View; }

	private:
		void CreateImage(void* data);
		void CreateDefaultImage(void* data);
		void CreateDynamicImage(void* data);
		void CreateStorageImage(void* data);
		void CreateFrameBufferImage(void* data);

		void CreateView();
		bool IsDepthImage();

	private:
		ImageSpecification m_Specs;

		ID3D11Texture2D* m_Resource = nullptr;
		ID3D11ShaderResourceView* m_View = nullptr;

		friend class DirectXRenderer;
	};

}
