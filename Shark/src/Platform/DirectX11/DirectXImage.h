#pragma once

#include "Shark/Core/Buffer.h"
#include "Shark/Render/Image.h"

#include <d3d11.h>

#undef UpdateResource

namespace Shark {

	namespace DXImageUtils {

		DXGI_FORMAT ImageFormatToD3D11ForResource(ImageFormat format);
		DXGI_FORMAT ImageFormatToD3D11ForView(ImageFormat format);
		D3D11_USAGE UsageFromImageType(ImageType imageType);
		UINT CPUAccessFromType(ImageType imageType);

	}

	class DirectXSwapChain;

	struct DirectXImageInfo
	{
		ID3D11Texture2D* Resource = nullptr;
		ID3D11ShaderResourceView* View = nullptr;
		ID3D11UnorderedAccessView* AccessView = nullptr;
	};

	class DirectXImage2D : public Image2D
	{
	public:
		DirectXImage2D();
		DirectXImage2D(const ImageSpecification& specs);
		virtual ~DirectXImage2D();

		virtual void Release() override;
		virtual void Invalidate() override;
		virtual void RT_Invalidate() override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual bool IsValid(bool hasaView = true) const override;

		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }
		virtual float GetAspectRatio() const override { return (float)GetWidth() / (float)GetHeight(); }
		virtual float GetVerticalAspectRatio() const override { return (float)GetHeight() / (float)GetWidth(); }

		virtual void UploadImageData(Buffer buffer) override;
		virtual void RT_UploadImageData(Buffer buffer) override;

		virtual Ref<Image2D> RT_GetStorageImage() override;
		virtual bool RT_ReadPixel(uint32_t x, uint32_t y, uint32_t& out_Pixel) override;

		virtual RenderID GetViewID() const override { return m_Info.View; }
		virtual ImageType GetType() const override { return m_Specification.Type; }

		virtual ImageSpecification& GetSpecification() override { return m_Specification; }
		virtual const ImageSpecification& GetSpecification() const override { return m_Specification; }

		DirectXImageInfo& GetDirectXImageInfo() { return m_Info; }
		const DirectXImageInfo& GetDirectXImageInfo() const { return m_Info; }

		void RT_CreateUnorderAccessView(uint32_t mipSlice);

	private:
		ImageSpecification m_Specification;
		DirectXImageInfo m_Info;

		friend class DirectXRenderer;
	};

	class DirectXImageView : public ImageView
	{
	public:
		DirectXImageView(Ref<Image2D> image, uint32_t mipSlice);
		~DirectXImageView();

		void Invalidate();

		virtual RenderID GetViewID() const override { return m_View; }
	private:
		Ref<Image2D> m_Image;
		uint32_t m_MipSlice;

		ID3D11ShaderResourceView* m_View = nullptr;

		std::string m_DebugName;
	};

}
