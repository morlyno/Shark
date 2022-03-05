#include "skpch.h"
#include "DirectXImage.h"

#include "Platform/DirectX11/DirectXRenderer.h"

#include <stb_image.h>

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR(SK_STRINGIFY(call) "0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	namespace utils {

		DXGI_FORMAT ImageFormatToD3D11ForResource(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None: return DXGI_FORMAT_UNKNOWN;
				case ImageFormat::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
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
				case ImageType::Default: return D3D11_USAGE_DEFAULT;
				case ImageType::Dynamic: return D3D11_USAGE_DYNAMIC;
				case ImageType::Storage: return D3D11_USAGE_STAGING;
				case ImageType::FrameBuffer: return D3D11_USAGE_DEFAULT;
			}
			SK_CORE_ASSERT(false, "Unkown Image Type");
			return D3D11_USAGE_DEFAULT;
		}

		UINT CPUAccessFromType(ImageType imageType)
		{
			if (imageType == ImageType::Dynamic)
				return D3D11_CPU_ACCESS_WRITE;
			if (imageType == ImageType::Storage)
				return D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
			return 0;
		}

	}

	DirectXImage2D::DirectXImage2D()
	{
	}

	DirectXImage2D::DirectXImage2D(const ImageSpecification& specs, void* data)
		: m_Specs(specs)
	{
		CreateImage(data);
	}

	DirectXImage2D::DirectXImage2D(ImageFormat format, uint32_t width, uint32_t height, void* data)
	{
		m_Specs.Format = format;
		m_Specs.Width = width;
		m_Specs.Height = height;

		CreateImage(data);
	}

	DirectXImage2D::~DirectXImage2D()
	{
		if (m_Resource)
			m_Resource->Release();
		if (m_View)
			m_View->Release();
	}

	void DirectXImage2D::Set(const ImageSpecification& specs, void* data)
	{
		if (m_Resource)
		{
			m_Resource->Release();
			m_Resource = nullptr;
		}
		if (m_View)
		{
			m_View->Release();
			m_View = nullptr;
		}

		m_Specs = specs;
		CreateImage(data);
	}

	void DirectXImage2D::Resize(uint32_t width, uint32_t height)
	{
		if (m_Specs.Width == width && m_Specs.Height == height)
			return;

		m_Specs.Width = width;
		m_Specs.Height = height;

		if (m_View)
		{
			m_View->Release();
			m_View = nullptr;
		}

		if (m_Resource)
		{
			m_Resource->Release();
			m_Resource = nullptr;
		}

		CreateImage(nullptr);
	}

	bool DirectXImage2D::CopyTo(Ref<Image2D> image)
	{
		Ref<DirectXImage2D> dxImage = image.As<DirectXImage2D>();
		SK_CORE_ASSERT(m_Resource && dxImage->m_Resource);

		const auto& destSpecs = image->GetSpecification();
		if (m_Resource != dxImage->m_Resource && m_Specs.Format != destSpecs.Format && m_Specs.Type == destSpecs.Type &&
			m_Specs.Width == destSpecs.Width && m_Specs.Height == destSpecs.Height)
		{
			SK_CORE_WARN("Unable to Copy Resource!");
			return false;
		}

		auto ctx = DirectXRenderer::GetContext();
		ctx->CopyResource(dxImage->m_Resource, m_Resource);
		return true;
	}

	bool DirectXImage2D::ReadPixel(uint32_t x, uint32_t y, uint32_t& out_Pixel)
	{
		if (m_Specs.Type != ImageType::Storage)
		{
			SK_CORE_WARN("Read Pixel only works with ImageType::Sotrage!");
			return false;
		}

		auto ctx = DirectXRenderer::GetContext();
		D3D11_MAPPED_SUBRESOURCE ms;
		HRESULT hr = ctx->Map(m_Resource, 0, D3D11_MAP_READ, 0, &ms);
		if (FAILED(hr))
		{
			SK_CORE_ERROR("Failed to map Resource");
			return false;
		}

		uint32_t* data = (uint32_t*)ms.pData;
		uint32_t padding = ms.RowPitch / 4;
		out_Pixel = data[y * padding + x];

		ctx->Unmap(m_Resource, 0);

		return true;
	}

	void DirectXImage2D::CreateImage(void* data)
	{
		SK_CORE_ASSERT(!m_Resource);
		SK_CORE_ASSERT(!m_View);

		switch (m_Specs.Type)
		{
			case ImageType::Default: CreateDefaultImage(data); break;
			case ImageType::Dynamic: CreateDynamicImage(data); break;
			case ImageType::Storage: CreateStorageImage(data); break;
			case ImageType::FrameBuffer: CreateFrameBufferImage(data); break;
			default: SK_CORE_ASSERT(false, "Unkown ImageType"); return;
		}

		if (m_Specs.Type != ImageType::Storage)
			CreateView();
	}

	void DirectXImage2D::CreateDefaultImage(void* data)
	{
		SK_CORE_ASSERT(m_Specs.Type == ImageType::Default);

		auto device = DirectXRenderer::GetDevice();

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = m_Specs.Width;
		desc.Height = m_Specs.Height;
		desc.MipLevels = m_Specs.MipLevels;
		desc.ArraySize = 1;
		desc.Format = utils::ImageFormatToD3D11ForResource(m_Specs.Format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		SK_CHECK(device->CreateTexture2D(&desc, nullptr, &m_Resource));
		if (data)
		{
			auto context = DirectXRenderer::GetContext();
			context->UpdateSubresource(m_Resource, 0, nullptr, data, m_Specs.Width * 4, 0);
		}

	}

	void DirectXImage2D::CreateDynamicImage(void* data)
	{
		SK_CORE_ASSERT(m_Specs.Type == ImageType::Dynamic);

		auto device = DirectXRenderer::GetDevice();

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = m_Specs.Width;
		desc.Height = m_Specs.Height;
		desc.MipLevels = m_Specs.MipLevels;
		desc.ArraySize = 1;
		desc.Format = utils::ImageFormatToD3D11ForResource(m_Specs.Format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		SK_CHECK(device->CreateTexture2D(&desc, nullptr, &m_Resource));
		if (data)
		{
			auto context = DirectXRenderer::GetContext();
			context->UpdateSubresource(m_Resource, 0, nullptr, data, m_Specs.Width * 4, 0);
		}

	}

	void DirectXImage2D::CreateStorageImage(void* data)
	{
		SK_CORE_ASSERT(m_Specs.Type == ImageType::Storage);

		auto device = DirectXRenderer::GetDevice();

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = m_Specs.Width;
		desc.Height = m_Specs.Height;
		desc.MipLevels = m_Specs.MipLevels;
		desc.ArraySize = 1;
		desc.Format = utils::ImageFormatToD3D11ForResource(m_Specs.Format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		desc.Usage = D3D11_USAGE_STAGING;
		desc.BindFlags = 0;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

		SK_CHECK(device->CreateTexture2D(&desc, nullptr, &m_Resource));
		if (data)
		{
			auto context = DirectXRenderer::GetContext();
			context->UpdateSubresource(m_Resource, 0, nullptr, data, m_Specs.Width * 4, 0);
		}

	}

	void DirectXImage2D::CreateFrameBufferImage(void* data)
	{
		SK_CORE_ASSERT(m_Specs.Type == ImageType::FrameBuffer);

		auto device = DirectXRenderer::GetDevice();

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = m_Specs.Width;
		desc.Height = m_Specs.Height;
		desc.MipLevels = m_Specs.MipLevels;
		desc.ArraySize = 1;
		desc.Format = utils::ImageFormatToD3D11ForResource(m_Specs.Format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.BindFlags |= IsDepthImage() ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;

		SK_CHECK(device->CreateTexture2D(&desc, nullptr, &m_Resource));
		if (data)
		{
			auto context = DirectXRenderer::GetContext();
			context->UpdateSubresource(m_Resource, 0, nullptr, data, m_Specs.Width * 4, 0);
		}

	}

	void DirectXImage2D::CreateView()
	{
		auto device = DirectXRenderer::GetDevice();

		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
		viewDesc.Format = utils::ImageFormatToD3D11ForView(m_Specs.Format);
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipLevels = -1;
		viewDesc.Texture2D.MostDetailedMip = 0;

		SK_CHECK(device->CreateShaderResourceView(m_Resource, &viewDesc, &m_View));
	}

	bool DirectXImage2D::IsDepthImage()
	{
		return m_Specs.Format == ImageFormat::Depth32;
	}

}
