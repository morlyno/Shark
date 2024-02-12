#include "skpch.h"
#include "DirectXImage.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Texture.h"
#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXRenderer.h"

#include <stb_image.h>

namespace Shark {

	namespace DXImageUtils {

		DXGI_FORMAT ImageFormatToD3D11ForResource(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None: return DXGI_FORMAT_UNKNOWN;
				case ImageFormat::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
				case ImageFormat::RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
				case ImageFormat::RGB32F: return DXGI_FORMAT_R32G32B32_FLOAT;
				case ImageFormat::RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
				case ImageFormat::R8: return DXGI_FORMAT_R8_UNORM;
				case ImageFormat::R16F: return DXGI_FORMAT_R16_FLOAT;
				case ImageFormat::R32_SINT: return DXGI_FORMAT_R32_SINT;
				case ImageFormat::Depth32: return DXGI_FORMAT_R32_TYPELESS;
			}
			SK_CORE_ASSERT(false, "Unkown Image Format");
			return DXGI_FORMAT_UNKNOWN;
		}

		DXGI_FORMAT ImageFormatToD3D11ForView(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None: return DXGI_FORMAT_UNKNOWN;
				case ImageFormat::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
				case ImageFormat::RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
				case ImageFormat::RGB32F: return DXGI_FORMAT_R32G32B32_FLOAT;
				case ImageFormat::RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
				case ImageFormat::R8: return DXGI_FORMAT_R8_UNORM;
				case ImageFormat::R16F: return DXGI_FORMAT_R16_FLOAT;
				case ImageFormat::R32_SINT: return DXGI_FORMAT_R32_SINT;
				case ImageFormat::Depth32: return DXGI_FORMAT_R32_FLOAT;
			}
			SK_CORE_ASSERT(false, "Unkown Image Format");
			return DXGI_FORMAT_UNKNOWN;
		}

		D3D11_USAGE UsageFromImageType(ImageType imageType)
		{
			switch (imageType)
			{
				case ImageType::Texture: return D3D11_USAGE_DEFAULT;
				//case ImageType::Dynamic: return D3D11_USAGE_DYNAMIC;
				case ImageType::Storage: return D3D11_USAGE_STAGING;
				case ImageType::Atachment: return D3D11_USAGE_DEFAULT;
			}
			SK_CORE_ASSERT(false, "Unkown Image Type");
			return D3D11_USAGE_DEFAULT;
		}

		UINT CPUAccessFromType(ImageType imageType)
		{
			//if (imageType == ImageType::Dynamic)
			//	return D3D11_CPU_ACCESS_WRITE;
			if (imageType == ImageType::Storage)
				return D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
			return 0;
		}

	}

	namespace utils {

		static std::string GenerateDebugName(const ImageSpecification& specification)
		{
			return fmt::format("{} - {} ({}, {}) Mips: {}", ToString(specification.Type), ToString(specification.Format), specification.Width, specification.Height, specification.MipLevels ? fmt::to_string(specification.MipLevels) : "Max");
		}

	}

	DirectXImage2D::DirectXImage2D()
	{
	}

	DirectXImage2D::DirectXImage2D(const ImageSpecification& specs)
		: m_Specification(specs)
	{
		RT_Invalidate();
	}

	DirectXImage2D::~DirectXImage2D()
	{
		Release();
	}

	void DirectXImage2D::Release()
	{
		if (!m_Info.Resource && !m_Info.View && !m_Info.AccessView)
			return;

		Renderer::SubmitResourceFree([info = m_Info]()
		{
			if (info.Resource)
				info.Resource->Release();
			if (info.View)
				info.View->Release();
			if (info.AccessView)
				info.AccessView->Release();
		});

		m_Info.Resource = nullptr;
		m_Info.View = nullptr;
		m_Info.AccessView = nullptr;
	}

	void DirectXImage2D::Invalidate()
	{
		if (m_Specification.MipLevels == 0)
		{
			m_Specification.MipLevels = ImageUtils::CalcMipLevels(m_Specification.Width, m_Specification.Height);
		}

		Ref<DirectXImage2D> instance = this;
		Renderer::Submit([instance]() { instance->RT_Invalidate(); });
	}

	void DirectXImage2D::RT_Invalidate()
	{
		Release();

		if (m_Specification.MipLevels == 0)
			m_Specification.MipLevels = ImageUtils::CalcMipLevels(m_Specification.Width, m_Specification.Height);

		if (m_Specification.DebugName.empty())
			m_Specification.DebugName = utils::GenerateDebugName(m_Specification);

		D3D11_TEXTURE2D_DESC texture2dDesc{};
		texture2dDesc.Width = m_Specification.Width;
		texture2dDesc.Height = m_Specification.Height;
		texture2dDesc.Format = DXImageUtils::ImageFormatToD3D11ForResource(m_Specification.Format);
		texture2dDesc.MipLevels = m_Specification.MipLevels;
		texture2dDesc.ArraySize = m_Specification.Layers;
		texture2dDesc.SampleDesc.Count = 1;
		texture2dDesc.SampleDesc.Quality = 0;
		switch (m_Specification.Type)
		{
			case ImageType::Texture:
				texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
				texture2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
				break;
			case ImageType::TextureCube:
				texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
				texture2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
				texture2dDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
				SK_CORE_VERIFY(m_Specification.Layers == 6);
				break;
			case ImageType::Atachment:
				texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
				texture2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (ImageUtils::IsDepthFormat(m_Specification.Format) ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET);
				break;
			case ImageType::Storage:
				texture2dDesc.Usage = D3D11_USAGE_STAGING;
				texture2dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
				break;
		}

		auto renderer = DirectXRenderer::Get();
		auto device = renderer->GetDevice();
		DirectXAPI::CreateTexture2D(device, texture2dDesc, nullptr, m_Info.Resource);
		DirectXAPI::SetDebugName(m_Info.Resource, m_Specification.DebugName);

		if (m_Specification.Type != ImageType::Storage)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Format = DXImageUtils::ImageFormatToD3D11ForView(m_Specification.Format);
			if (m_Specification.Type == ImageType::TextureCube)
			{
				shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				shaderResourceViewDesc.TextureCube.MipLevels = -1;
				shaderResourceViewDesc.TextureCube.MostDetailedMip = 0;
			}
			else if (m_Specification.Layers > 1)
			{
				shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				shaderResourceViewDesc.Texture2DArray.MipLevels = -1;
				shaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
				shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
				shaderResourceViewDesc.Texture2DArray.ArraySize = m_Specification.Layers;
			}
			else
			{
				shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				shaderResourceViewDesc.Texture2D.MipLevels = -1;
				shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
			}
			DirectXAPI::CreateShaderResourceView(device, m_Info.Resource, shaderResourceViewDesc, m_Info.View);
			DirectXAPI::SetDebugName(m_Info.View, m_Specification.DebugName);
		}
	}

	void DirectXImage2D::Resize(uint32_t width, uint32_t height)
	{
		if (m_Specification.Width == width && m_Specification.Height == height)
			return;

		m_Specification.Width = width;
		m_Specification.Height = height;

		Ref<DirectXImage2D> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_Invalidate();
		});
	}

	bool DirectXImage2D::IsValid(bool hasView) const
	{
		return hasView ? m_Info.View != nullptr : m_Info.Resource != nullptr;
	}

	void DirectXImage2D::UploadImageData(Buffer buffer)
	{
		Ref<DirectXImage2D> instance = this;
		Renderer::Submit([instance, imageData = Buffer::Copy(buffer)] () mutable
		{
			instance->RT_UploadImageData(imageData);
			imageData.Release();
		});
	}

	void DirectXImage2D::RT_UploadImageData(Buffer imageData)
	{
		const uint64_t memoryUsage = (uint64_t)m_Specification.Width * m_Specification.Height * m_Specification.Layers * ImageUtils::GetFormatDataSize(m_Specification.Format);
		SK_CORE_VERIFY(imageData.Size == memoryUsage, "{} == {}", imageData.Size, memoryUsage);

		auto context = DirectXRenderer::GetContext();
		const uint32_t formatDataSize = ImageUtils::GetFormatDataSize(m_Specification.Format);
		context->UpdateSubresource(m_Info.Resource, 0, nullptr, imageData.As<const void*>(), m_Specification.Width * formatDataSize, 0);
	}

	Ref<Image2D> DirectXImage2D::RT_GetStorageImage()
	{
		auto storageImage = Image2D::Create();
		auto& specification = storageImage->GetSpecification();
		specification.Width = m_Specification.Width;
		specification.Height = m_Specification.Height;
		specification.Format = m_Specification.Format;
		specification.MipLevels = m_Specification.MipLevels;
		specification.Type = ImageType::Storage;
		storageImage->RT_Invalidate();
		Renderer::CopyImage(Renderer::GetCommandBuffer(), this, storageImage);
		return storageImage;
	}

	bool DirectXImage2D::RT_ReadPixel(uint32_t x, uint32_t y, uint32_t& out_Pixel)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		SK_CORE_VERIFY(m_Specification.Type == ImageType::Storage);

		if (x >= m_Specification.Width || y >= m_Specification.Height)
			return false;

		auto ctx = DirectXRenderer::GetContext();
		D3D11_MAPPED_SUBRESOURCE ms;
		HRESULT hr = ctx->Map(m_Info.Resource, 0, D3D11_MAP_READ, 0, &ms);
		DX11_VERIFY(hr);
		if (FAILED(hr))
			return false;

		uint32_t* data = (uint32_t*)ms.pData;
		uint32_t padding = ms.RowPitch / 4;
		out_Pixel = data[y * padding + x];

		ctx->Unmap(m_Info.Resource, 0);

		return true;
	}

	void DirectXImage2D::RT_CreateUnorderAccessView(uint32_t mipSlice)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
		unorderedAccessViewDesc.Format = DXImageUtils::ImageFormatToD3D11ForView(m_Specification.Format);
		if (m_Specification.Type == ImageType::TextureCube)
		{
			unorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
			unorderedAccessViewDesc.Texture2DArray.MipSlice = mipSlice;
			unorderedAccessViewDesc.Texture2DArray.FirstArraySlice = 0;
			unorderedAccessViewDesc.Texture2DArray.ArraySize = m_Specification.Layers;
		}
		else
		{
			unorderedAccessViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			unorderedAccessViewDesc.Texture2D.MipSlice = mipSlice;
		}

		Ref<DirectXRenderer> renderer = DirectXRenderer::Get();
		ID3D11Device* device = renderer->GetDevice();
		DirectXAPI::CreateUnorderedAccessView(device, m_Info.Resource, unorderedAccessViewDesc, m_Info.AccessView);
		DirectXAPI::SetDebugName(m_Info.AccessView, m_Specification.DebugName);
	}

	DirectXImageView::DirectXImageView(Ref<Image2D> image, uint32_t mipSlice)
		: m_Image(image), m_MipSlice(mipSlice)
	{
		Invalidate();
	}

	DirectXImageView::~DirectXImageView()
	{
		if (!m_View)
			return;

		Renderer::SubmitResourceFree([view = m_View]()
		{
			if (view)
				view->Release();
		});
		m_View = nullptr;
	}

	void DirectXImageView::Invalidate()
	{
		Ref<DirectXImage2D> dxImage = m_Image.As<DirectXImage2D>();
		const auto& imageSpecification = dxImage->GetSpecification();
		SK_CORE_VERIFY(imageSpecification.MipLevels != 0);
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
		shaderResourceViewDesc.Format = DXImageUtils::ImageFormatToD3D11ForView(dxImage->GetSpecification().Format);
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MipLevels = 1;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = m_MipSlice;

		Ref<DirectXRenderer> renderer = DirectXRenderer::Get();
		ID3D11Device* device = renderer->GetDevice();
		DirectXAPI::CreateShaderResourceView(device, dxImage->GetDirectXImageInfo().Resource, shaderResourceViewDesc, m_View);
		m_DebugName = fmt::format("{} (Mip: {})", dxImage->GetSpecification().DebugName, m_MipSlice);
		DirectXAPI::SetDebugName(m_View, m_DebugName);
	}

}
