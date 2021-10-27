#pragma once

#include "Shark/Render/Image.h"

#include <d3d11.h>

namespace Shark {

	namespace Utils {

		D3D11_USAGE ImageTypeToD3D(ImageType usage);

		DXGI_FORMAT ImageFormatToGXGI(ImageFormat format);

		uint32_t GetImageFormatSize(ImageFormat format);

		UINT ImageUsageToD3D(uint32_t flags);

	}


	class DirectXImage2D : public Image2D
	{
	public:
		DirectXImage2D(void* data, const ImageSpecification& specs);
		DirectXImage2D(const std::filesystem::path& filepath, const ImageSpecification& specs);
		virtual ~DirectXImage2D();

		virtual uint32_t GetWidth() const override { return m_Specs.Width; }
		virtual uint32_t GetHeight() const override { return m_Specs.Height; }

		virtual void Resize(uint32_t widht, uint32_t height) override;

		virtual void SetData(void* data, uint32_t size) override;
		virtual void CopyTo(Ref<Image2D> dest) override;

		virtual uint32_t ReadPixel(uint32_t x, uint32_t y) override;

		virtual void CreateView();
		virtual bool HasView() const { return m_View != nullptr; }

		ID3D11Texture2D* GetNative() const { return m_Image; }
		ID3D11ShaderResourceView* GetViewNative() const { return m_View; }
		virtual RenderID GetRenderID() const override { return GetNative(); }
		virtual RenderID GetViewRenderID() const override { return GetViewNative(); }
		virtual const ImageSpecification& GetSpecification() const override { return m_Specs; }

	private:
		void CreateImage(void* data);
		void CreateDefaultImage(void* data);
		void CreateDynamicImage(void* data);
		void CreateImmutableImage(void* data);
		void CreateStagingImage(void* data);

	private:
		ImageSpecification m_Specs;
		ID3D11Texture2D* m_Image = nullptr;
		ID3D11ShaderResourceView* m_View = nullptr;

		friend class DirectXRenderer;
	};

}
