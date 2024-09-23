#pragma once

#include "Shark/Core/Buffer.h"
#include "Shark/Render/Image.h"

#include <d3d11.h>

#undef UpdateResource

namespace Shark {

	namespace DXImageUtils {

		DXGI_FORMAT ImageFormatToDXGI(ImageFormat format);
		DXGI_FORMAT FixImageFormatForResource(DXGI_FORMAT format);
		DXGI_FORMAT FixImageFormatForView(DXGI_FORMAT format);

		D3D11_USAGE UsageFromImageType(ImageType imageType);
		UINT CPUAccessFromType(ImageType imageType);

	}

	class DirectXSwapChain;

	struct DirectXImageInfo
	{
		ID3D11Texture2D* Resource = nullptr;
		ID3D11ShaderResourceView* View = nullptr;
		ID3D11SamplerState* Sampler = nullptr;
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

		virtual bool RT_ReadPixel(uint32_t x, uint32_t y, uint32_t& out_Pixel) override;
		virtual void RT_CopyToHostBuffer(Buffer& buffer) override;

		virtual RenderID GetViewID() const override { return m_Info.View; }
		virtual ImageType GetType() const override { return m_Specification.Type; }

		virtual ImageSpecification& GetSpecification() override { return m_Specification; }
		virtual const ImageSpecification& GetSpecification() const override { return m_Specification; }

		DirectXImageInfo& GetDirectXImageInfo() { return m_Info; }
		const DirectXImageInfo& GetDirectXImageInfo() const { return m_Info; }
		std::map<uint32_t, ID3D11UnorderedAccessView*>& GetPerMipUAVs() { return m_PerMipUAVs; }
		const std::map<uint32_t, ID3D11UnorderedAccessView*>& GetPerMipUAVs() const { return m_PerMipUAVs; }

		void RT_CreateUnorderAccessView(uint32_t mipSlice);
		void RT_CreatePerMipUAV();
		ID3D11UnorderedAccessView*& GetUAV(uint32_t mipSlice) { return m_PerMipUAVs.at(mipSlice); }

	private:
		ImageSpecification m_Specification;
		DirectXImageInfo m_Info;

		std::map<uint32_t, ID3D11UnorderedAccessView*> m_PerMipUAVs;

		friend class DirectXRenderer;
	};

	struct DirectXViewInfo
	{
		ID3D11Resource* Resource = nullptr;
		ID3D11ShaderResourceView* View = nullptr;
		ID3D11SamplerState* Sampler = nullptr;
	};

	class DirectXImageView : public ImageView
	{
	public:
		DirectXImageView(Ref<Image2D> image, uint32_t mipSlice);
		~DirectXImageView();

		void Invalidate();

		virtual Ref<Image2D> GetImage() const { return m_Image; }
		virtual RenderID GetViewID() const override { return m_Info.View; }

		DirectXViewInfo& GetDirectXViewInfo() { return m_Info; }
		const DirectXViewInfo& GetDirectXViewInfo() const { return m_Info; }
	private:
		Ref<Image2D> m_Image;
		uint32_t m_MipSlice;

		DirectXViewInfo m_Info;

		std::string m_DebugName;

		friend class DirectXRenderer;
	};

}
