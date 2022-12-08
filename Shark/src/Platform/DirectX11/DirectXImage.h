#pragma once

#include "Shark/Core/Buffer.h"
#include "Shark/Render/Image.h"

#include <d3d11.h>

namespace Shark {

	namespace utils {

		DXGI_FORMAT ImageFormatToD3D11ForResource(ImageFormat format);
		DXGI_FORMAT ImageFormatToD3D11ForView(ImageFormat format);
		D3D11_USAGE UsageFromImageType(ImageType imageType);
		UINT CPUAccessFromType(ImageType imageType);

	}

	class DirectXSwapChain;

	class DirectXImage2D : public Image2D
	{
	public:
		DirectXImage2D();
		DirectXImage2D(const ImageSpecification& specs);
		DirectXImage2D(const ImageSpecification& specs, Buffer imageData);
		DirectXImage2D(const ImageSpecification& specs, Ref<Image2D> data);
		DirectXImage2D(ImageFormat format, uint32_t width, uint32_t height, Buffer imageData);
		DirectXImage2D(const std::filesystem::path& filePath);
		DirectXImage2D(const ImageSpecification& specs, ID3D11Texture2D* resource, bool createView);
		DirectXImage2D(Ref<DirectXSwapChain> swapchain, bool createView);
		virtual ~DirectXImage2D();

		void Release();
		void RT_Release();

		virtual bool IsValid() const override { return m_Resource && m_View; }

		virtual void Set(const ImageSpecification& specs, Buffer imageData) override;
		virtual void Set(const ImageSpecification& specs, Ref<Image2D> data) override;
		virtual void Set(const std::filesystem::path& filePath) override;
		
		virtual void ReloadFromDisc() override;

		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual bool CopyTo(Ref<Image2D> image) override;

		virtual bool ReadPixel(uint32_t x, uint32_t y, uint32_t& out_Pixel) override;

		virtual RenderID GetResourceID() const override { return m_Resource; }
		virtual RenderID GetViewID() const override { return m_View; }
		virtual const ImageSpecification& GetSpecification() const override { return m_Specification; }
		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }

		virtual const std::filesystem::path& GetFilePath() const override { return m_FilePath; }
		virtual void SetFilePath(const std::filesystem::path& filePath) override { m_FilePath = filePath; }

		ID3D11Texture2D* GetResourceNative() const { return m_Resource; }
		ID3D11ShaderResourceView* GetViewNative() const { return m_View; }

	public:
		virtual void RT_CopyTo(Ref<Image2D> image) override;

	private:
		Buffer LoadDataFromFile(const std::filesystem::path& filePath);

		void CreateResource();
		void UpdateResource(Buffer imageData);
		void UpdateResource(Ref<DirectXImage2D> imageData);

		void CreateView();

		bool IsDepthImage() { return m_Specification.Format == ImageFormat::Depth32; }
		bool IsImageCompadible(const ImageSpecification& specs) const { return m_Specification.Width == specs.Width && m_Specification.Height == specs.Height && m_Specification.Format == specs.Format && m_Specification.MipLevels == specs.MipLevels; }
		bool IsImageCompadibleIgnoreMipLeves(const ImageSpecification& specs) const { return m_Specification.Width == specs.Width && m_Specification.Height == specs.Height && m_Specification.Format == specs.Format; }

	private:
		void RT_CreateResource();
		void RT_UpdateResource(Buffer imageData);
		void RT_UpdateResource(Ref<DirectXImage2D> imageData);
		void RT_CreateView();

	private:
		ImageSpecification m_Specification;

		ID3D11Texture2D* m_Resource = nullptr;
		ID3D11ShaderResourceView* m_View = nullptr;

		std::filesystem::path m_FilePath;

		friend class DirectXRenderer;
	};

}
