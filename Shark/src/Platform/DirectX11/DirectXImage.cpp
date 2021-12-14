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

	namespace Utils {

		D3D11_USAGE ImageTypeToD3D(ImageType usage)
		{
			switch (usage)
			{
				case ImageType::Default:   return D3D11_USAGE_DEFAULT;
				case ImageType::Immutable: return D3D11_USAGE_IMMUTABLE;
				case ImageType::Dynamic:   return D3D11_USAGE_DYNAMIC;
				case ImageType::Staging:   return D3D11_USAGE_STAGING;
			}
			SK_CORE_ASSERT(false);
			return (D3D11_USAGE)0;
		}

		DXGI_FORMAT ImageFormatToGXGI(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None:               return DXGI_FORMAT_UNKNOWN;
				case ImageFormat::RGBA8:              return DXGI_FORMAT_R8G8B8A8_UNORM;
				case ImageFormat::R32_SINT:           return DXGI_FORMAT_R32_SINT;
				case ImageFormat::Depth32:            return DXGI_FORMAT_R32_TYPELESS; // D32_FLOAT can't be bound as ShaderResourceView but R32_TYPELESS Works
				case ImageFormat::SwapChain:          SK_CORE_ASSERT(false, "Invalid ImageFormat"); return DXGI_FORMAT_R8G8B8A8_UNORM;
			}
			SK_CORE_ASSERT(false);
			return DXGI_FORMAT_UNKNOWN;
		}

		DXGI_FORMAT ImageFormatToGXGI_ForView(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None:               return DXGI_FORMAT_UNKNOWN;
				case ImageFormat::RGBA8:              return DXGI_FORMAT_R8G8B8A8_UNORM;
				case ImageFormat::R32_SINT:           return DXGI_FORMAT_R32_SINT;
				case ImageFormat::Depth32:            return DXGI_FORMAT_R32_FLOAT;
				case ImageFormat::SwapChain:          SK_CORE_ASSERT(false, "Invalid ImageFormat"); return DXGI_FORMAT_R8G8B8A8_UNORM;
			}
			SK_CORE_ASSERT(false);
			return DXGI_FORMAT_UNKNOWN;
		}

		uint32_t GetImageFormatSize(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None:               return 0;
				case ImageFormat::RGBA8:              return 4;
				case ImageFormat::R32_SINT:           return 4;
				case ImageFormat::Depth32:            return 4;
				case ImageFormat::SwapChain:          SK_CORE_ASSERT(false, "Invalid ImageFormat"); return 4;
			}
			SK_CORE_ASSERT(false);
			return 0;
		}

		UINT ImageUsageToD3D(uint32_t flags)
		{
			UINT d3dflags = 0;
			if (flags & ImageUsageTexture)   d3dflags |= D3D11_BIND_SHADER_RESOURCE;
			if (flags & ImageUsageFrameBuffer)      d3dflags |= D3D11_BIND_RENDER_TARGET;
			if (flags & ImageUsageDethStencil)      d3dflags |= D3D11_BIND_DEPTH_STENCIL;
			return d3dflags;
		}

	}


	DirectXImage2D::DirectXImage2D()
	{
	}

	DirectXImage2D::DirectXImage2D(void* data, const ImageSpecification& specs)
		: m_Specs(specs)
	{
		CreateImage(data);
	}

	DirectXImage2D::DirectXImage2D(const std::filesystem::path& filepath, const ImageSpecification& specs)
		: m_Specs(specs)
	{
		SK_CORE_WARN("Image Loaded form: {}", filepath);

		std::string narrorFilePath = filepath.string();
		int x, y, comp;
		stbi_uc* data = stbi_load(narrorFilePath.c_str(), &x, &y, &comp, 4);
		// TODO(moro): fallback
		SK_CORE_ASSERT(data, fmt::format("Failed to load Image! {}", stbi_failure_reason()));

		m_Specs.Width = x;
		m_Specs.Height = y;
		m_Specs.Format = ImageFormat::RGBA8;

		CreateImage(data);

		stbi_image_free(data);
	}

	DirectXImage2D::~DirectXImage2D()
	{
		if (m_Image)
			m_Image->Release();
		if (m_View)
			m_View->Release();
	}

	void DirectXImage2D::Set(void* data, const ImageSpecification& specs)
	{
		m_Specs = specs;
		CreateImage(data);
	}

	void DirectXImage2D::Resize(uint32_t widht, uint32_t height)
	{
		SK_CORE_ASSERT(m_Specs.Type != ImageType::Immutable);

		if (m_Specs.Width == widht && m_Specs.Height == height)
			return;

		m_Specs.Width = widht;
		m_Specs.Height = height;

		bool hasView = m_View != nullptr;
		if (m_View)
		{
			m_View->Release();
			m_View = nullptr;
		}

		m_Image->Release();
		CreateImage(nullptr);

		if (hasView)
			CreateView();
	}

	void DirectXImage2D::SetData(void* data, uint32_t size)
	{
		SK_CORE_ASSERT(m_Specs.Type == ImageType::Dynamic || m_Specs.Type == ImageType::Staging);
		SK_CORE_ASSERT(size == m_Specs.Width * m_Specs.Height * Utils::GetImageFormatSize(m_Specs.Format));

		auto ctx = DirectXRenderer::GetContext();

		const D3D11_MAP mapMode = m_Specs.Type == ImageType::Dynamic ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE;

		D3D11_MAPPED_SUBRESOURCE ms;
		SK_CHECK(ctx->Map(m_Image, 0, mapMode, 0, &ms));
		memcpy(ms.pData, data, size);
		ctx->Unmap(m_Image, 0);
	}

	void DirectXImage2D::CopyTo(Ref<Image2D> dest)
	{
		SK_CORE_ASSERT(Ref(this) != dest);
		SK_CORE_ASSERT(dest->GetSpecification().Type != ImageType::Immutable);
		SK_CORE_ASSERT(m_Specs.Width == dest->GetSpecification().Width);
		SK_CORE_ASSERT(m_Specs.Height == dest->GetSpecification().Height);
		SK_CORE_ASSERT(m_Specs.Format == dest->GetSpecification().Format, "Until Format Groups are introduced the Formats must be the same");

		auto ctx = DirectXRenderer::GetContext();

		Ref<DirectXImage2D> dxDest = dest.As<DirectXImage2D>();

		ctx->CopyResource(dxDest->m_Image, m_Image);
	}

	uint32_t DirectXImage2D::ReadPixel(uint32_t x, uint32_t y)
	{
		SK_CORE_ASSERT(m_Specs.Type == ImageType::Staging);

		auto ctx = DirectXRenderer::GetContext();

		D3D11_MAPPED_SUBRESOURCE ms;
		ctx->Map(m_Image, 0, D3D11_MAP_READ, 0, &ms);

		uint32_t* data = (uint32_t*)ms.pData;
		uint32_t padding = ms.RowPitch / Utils::GetImageFormatSize(m_Specs.Format);
		uint32_t val = data[y * padding + x];

		ctx->Unmap(m_Image, 0);

		return val;
	}

	void DirectXImage2D::CreateView()
	{
		SK_CORE_ASSERT(m_View == nullptr);

		auto* dev = DirectXRenderer::GetDevice();

		D3D11_SHADER_RESOURCE_VIEW_DESC srv;
		srv.Format = Utils::ImageFormatToGXGI_ForView(m_Specs.Format);
		srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = 1u;
		srv.Texture2D.MostDetailedMip = 0u;

		SK_CHECK(dev->CreateShaderResourceView(m_Image, &srv, &m_View));
	}

	void DirectXImage2D::CreateImage(void* data)
	{
		SK_CORE_ASSERT(m_Specs.Width != 0 && m_Specs.Height != 0);

		switch (m_Specs.Type)
		{
			case ImageType::Default: CreateDefaultImage(data); break;
			case ImageType::Immutable: CreateImmutableImage(data); break;
			case ImageType::Dynamic: CreateDynamicImage(data); break;
			case ImageType::Staging: CreateStagingImage(data); break;
			default: SK_CORE_ASSERT(false, "Unkonw Image Usage"); break;
		}
	}

	void DirectXImage2D::CreateDefaultImage(void* data)
	{
		SK_CORE_ASSERT(m_Specs.Type == ImageType::Default);

		auto dev = DirectXRenderer::GetDevice();

		D3D11_TEXTURE2D_DESC td;
		td.Width = m_Specs.Width;
		td.Height = m_Specs.Height;
		td.MipLevels = 1u;
		td.ArraySize = 1u;
		td.Format = Utils::ImageFormatToGXGI(m_Specs.Format);
		td.SampleDesc.Count = 1u;
		td.SampleDesc.Quality = 0u;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = Utils::ImageUsageToD3D(m_Specs.Usage);
		td.CPUAccessFlags = 0;
		td.MiscFlags = 0u;

		if (data)
		{
			D3D11_SUBRESOURCE_DATA srd;
			srd.pSysMem = data;
			srd.SysMemPitch = Utils::GetImageFormatSize(m_Specs.Format) * m_Specs.Width;
			srd.SysMemSlicePitch = 0;

			SK_CHECK(dev->CreateTexture2D(&td, &srd, &m_Image));
		}
		else
		{
			SK_CHECK(dev->CreateTexture2D(&td, nullptr, &m_Image));
		}

	}

	void DirectXImage2D::CreateDynamicImage(void* data)
	{
		SK_CORE_ASSERT(m_Specs.Type == ImageType::Dynamic);

		auto dev = DirectXRenderer::GetDevice();

		D3D11_TEXTURE2D_DESC td;
		td.Width = m_Specs.Width;
		td.Height = m_Specs.Height;
		td.MipLevels = 1u;
		td.ArraySize = 1u;
		td.Format = Utils::ImageFormatToGXGI(m_Specs.Format);
		td.SampleDesc.Count = 1u;
		td.SampleDesc.Quality = 0u;
		td.Usage = D3D11_USAGE_DYNAMIC;
		td.BindFlags = Utils::ImageUsageToD3D(m_Specs.Usage);
		td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		td.MiscFlags = 0u;

		if (data)
		{
			D3D11_SUBRESOURCE_DATA srd;
			srd.pSysMem = data;
			srd.SysMemPitch = Utils::GetImageFormatSize(m_Specs.Format) * m_Specs.Width;
			srd.SysMemSlicePitch = 0;

			SK_CHECK(dev->CreateTexture2D(&td, &srd, &m_Image));
		}
		else
		{
			SK_CHECK(dev->CreateTexture2D(&td, nullptr, &m_Image));
		}

	}

	void DirectXImage2D::CreateImmutableImage(void* data)
	{
		SK_CORE_ASSERT(data);
		SK_CORE_ASSERT(m_Specs.Type == ImageType::Immutable);

		auto dev = DirectXRenderer::GetDevice();

		D3D11_TEXTURE2D_DESC td;
		td.Width = m_Specs.Width;
		td.Height = m_Specs.Height;
		td.MipLevels = 1u;
		td.ArraySize = 1u;
		td.Format = Utils::ImageFormatToGXGI(m_Specs.Format);
		td.SampleDesc.Count = 1u;
		td.SampleDesc.Quality = 0u;
		td.Usage = D3D11_USAGE_IMMUTABLE;
		td.BindFlags = Utils::ImageUsageToD3D(m_Specs.Usage);
		td.CPUAccessFlags = 0;
		td.MiscFlags = 0u;

		D3D11_SUBRESOURCE_DATA srd;
		srd.pSysMem = data;
		srd.SysMemPitch = Utils::GetImageFormatSize(m_Specs.Format) * m_Specs.Width;
		srd.SysMemSlicePitch = 0;

		SK_CHECK(dev->CreateTexture2D(&td, &srd, &m_Image));

	}

	void DirectXImage2D::CreateStagingImage(void* data)
	{
		SK_CORE_ASSERT(m_Specs.Type == ImageType::Staging);

		auto dev = DirectXRenderer::GetDevice();

		D3D11_TEXTURE2D_DESC td;
		td.Width = m_Specs.Width;
		td.Height = m_Specs.Height;
		td.MipLevels = 1u;
		td.ArraySize = 1u;
		td.Format = Utils::ImageFormatToGXGI(m_Specs.Format);
		td.SampleDesc.Count = 1u;
		td.SampleDesc.Quality = 0u;
		td.Usage = D3D11_USAGE_STAGING;
		td.BindFlags = Utils::ImageUsageToD3D(m_Specs.Usage);
		td.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		td.MiscFlags = 0u;

		if (data)
		{
			D3D11_SUBRESOURCE_DATA srd;
			srd.pSysMem = data;
			srd.SysMemPitch = Utils::GetImageFormatSize(m_Specs.Format) * m_Specs.Width;
			srd.SysMemSlicePitch = 0;

			SK_CHECK(dev->CreateTexture2D(&td, &srd, &m_Image));
		}
		else
		{
			SK_CHECK(dev->CreateTexture2D(&td, nullptr, &m_Image));
		}

	}

}
