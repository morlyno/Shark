#include "skpch.h"
#include "DirectXImage.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Texture.h"
#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXRenderer.h"

#include <stb_image.h>

namespace Shark {

	namespace utils {

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
				case ImageType::FrameBuffer: return D3D11_USAGE_DEFAULT;
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

		uint32_t GetFormatDataSize(ImageFormat imageFormat)
		{
			switch (imageFormat)
			{
				case ImageFormat::None: return 0;
				case ImageFormat::RGBA8: return 4;
				case ImageFormat::RGBA16F: return 8; // 4 * 2bytes
				case ImageFormat::RGBA32F: return 4 * 4; // 4 * 4bytes
				case ImageFormat::RGB32F: return 3 * 4; // 3 * 4bytes
				case ImageFormat::R8: return 1;
				case ImageFormat::R16F: return 2;
				case ImageFormat::R32_SINT: return 4;
				case ImageFormat::Depth32: return 4;
			}
			SK_CORE_ASSERT(false, "Unkown ImageFormat");
			return 0;
		}

		static std::string GenerateDebugName(const ImageSpecification& specification)
		{
			return fmt::format("{} - {} ({}, {}) Mips: {}", ToString(specification.Type), ToString(specification.Format), specification.Width, specification.Height, specification.MipLevels ? fmt::to_string(specification.MipLevels) : "Max");
		}

		static bool IsDepthFormat(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::Depth32:
					return true;
			}
			return false;
		}

	}

	DirectXImage2D::DirectXImage2D()
	{
	}

	DirectXImage2D::DirectXImage2D(const ImageSpecification& specs)
		: m_Specification(specs)
	{
		Invalidate();
	}

	DirectXImage2D::DirectXImage2D(const ImageSpecification& specs, Ref<Image2D> data)
		: m_Specification(specs)
	{
		Invalidate();
		UpdateResource(data.As<DirectXImage2D>());
	}

	DirectXImage2D::DirectXImage2D(const ImageSpecification& spec, ID3D11Texture2D* resource, bool createView)
		: m_Specification(spec), m_Resource(resource)
	{
		if (createView)
		{
			Ref<DirectXImage2D> instance = this;
			Renderer::Submit([instance]() { instance->RT_CreateView(); });
		}
	}

	DirectXImage2D::~DirectXImage2D()
	{
		Release();
	}

	void DirectXImage2D::Invalidate()
	{
		Ref<DirectXImage2D> instance = this;
		Renderer::Submit([instance]() { instance->RT_Invalidate(); });
	}

	void DirectXImage2D::RT_Invalidate()
	{
		RT_Release();

		D3D11_TEXTURE2D_DESC texture2dDesc{};
		texture2dDesc.Width = m_Specification.Width;
		texture2dDesc.Height = m_Specification.Height;
		texture2dDesc.Format = utils::ImageFormatToD3D11ForResource(m_Specification.Format);
		texture2dDesc.MipLevels = m_Specification.MipLevels;
		texture2dDesc.ArraySize = 1;
		texture2dDesc.SampleDesc.Count = 1;
		texture2dDesc.SampleDesc.Quality = 0;
		switch (m_Specification.Type)
		{
			case ImageType::Texture:
				texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
				texture2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				break;
			case ImageType::FrameBuffer:
				texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
				texture2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (utils::IsDepthFormat(m_Specification.Format) ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET);
				break;
			case ImageType::Storage:
				texture2dDesc.Usage = D3D11_USAGE_STAGING;
				texture2dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
				break;
		}

		auto renderer = DirectXRenderer::Get();
		auto device = renderer->GetDevice();
		DirectXAPI::CreateTexture2D(device, texture2dDesc, nullptr, m_Resource);

		if (m_Specification.DebugName.empty())
			m_Specification.DebugName = utils::GenerateDebugName(m_Specification);
		DX11_VERIFY(D3D_SET_OBJECT_NAME_A(m_Resource, m_Specification.DebugName.c_str()));

		if (m_Specification.Type != ImageType::Storage)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Format = utils::ImageFormatToD3D11ForView(m_Specification.Format);
			shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			shaderResourceViewDesc.Texture2D.MipLevels = -1;
			shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
			DirectXAPI::CreateShaderResourceView(device, m_Resource, shaderResourceViewDesc, m_View);
			DX11_VERIFY(D3D_SET_OBJECT_NAME_A(m_View, m_Specification.DebugName.c_str()));
		}

	}

	bool DirectXImage2D::Validate(bool hasView) const
	{
		if (!hasView || m_Specification.Type == ImageType::Storage)
			return m_Resource;

		return m_Resource && m_View;
	}

	void DirectXImage2D::Release()
	{
		if (!m_Resource && !m_View)
			return;

		Renderer::SubmitResourceFree([resource = m_Resource, view = m_View]()
		{
			if (resource)
				resource->Release();
			if (view)
				view->Release();
		});

		m_Resource = nullptr;
		m_View = nullptr;
	}

	void DirectXImage2D::RT_Release()
	{
		if (m_Resource)
			m_Resource->Release();
		if (m_View)
			m_View->Release();

		m_Resource = nullptr;
		m_View = nullptr;
	}

	void DirectXImage2D::Resize(uint32_t width, uint32_t height)
	{
		if (m_Specification.Width == width && m_Specification.Height == height)
			return;

		m_Specification.Width = width;
		m_Specification.Height = height;

		if (m_Specification.DebugName.empty())
			m_Specification.DebugName = utils::GenerateDebugName(m_Specification);

		Ref<DirectXImage2D> instance = this;
		Renderer::Submit([instance, format = m_Specification.Format, width, height, mips = m_Specification.MipLevels, type = m_Specification.Type]()
		{
			auto renderer = DirectXRenderer::Get();
			ID3D11Device* device = renderer->GetDevice();

			instance->RT_Release();

			D3D11_TEXTURE2D_DESC texture2dDesc{};
			texture2dDesc.Width = width;
			texture2dDesc.Height = height;
			texture2dDesc.Format = utils::ImageFormatToD3D11ForResource(format);
			texture2dDesc.MipLevels = mips;
			texture2dDesc.ArraySize = 1;
			texture2dDesc.SampleDesc.Count = 1;
			texture2dDesc.SampleDesc.Quality = 0;
			switch (type)
			{
				case ImageType::Texture:
					texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
					texture2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					break;
				case ImageType::FrameBuffer:
					texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
					texture2dDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | (utils::IsDepthFormat(format) ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET);
					break;
				case ImageType::Storage:
					texture2dDesc.Usage = D3D11_USAGE_STAGING;
					texture2dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
					break;
			}

			ID3D11Texture2D* resource = nullptr;
			DirectXAPI::CreateTexture2D(device, texture2dDesc, nullptr, resource);
			DirectXAPI::SetDebugName(resource, instance->m_Specification.DebugName.c_str());
			instance->m_Resource = resource;

			if (type != ImageType::Storage)
			{
				D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
				shaderResourceViewDesc.Format = utils::ImageFormatToD3D11ForView(format);
				shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				shaderResourceViewDesc.Texture2D.MipLevels = -1;
				shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;

				ID3D11ShaderResourceView* view = nullptr;
				DirectXAPI::CreateShaderResourceView(device, resource, shaderResourceViewDesc, view);
				DirectXAPI::SetDebugName(view, instance->m_Specification.DebugName.c_str());
				instance->m_View = view;
			}

		});
	}

	void DirectXImage2D::UploadImageData(Buffer buffer)
	{
		UpdateResource(buffer);
	}

	void DirectXImage2D::RT_UploadImageData(Buffer buffer)
	{
		RT_UpdateResource(buffer);
	}

	void DirectXImage2D::UploadImageData(Ref<Image2D> image)
	{
		UpdateResource(image.As<DirectXImage2D>());
	}

	void DirectXImage2D::RT_UploadImageData(Ref<Image2D> image)
	{
		RT_UpdateResource(image.As<DirectXImage2D>());
	}

	Ref<Image2D> DirectXImage2D::RT_GetStorageImage()
	{
		auto storageImage = Image2D::Create();
		auto& specification = storageImage->GetSpecificationMutable();
		specification.Width = m_Specification.Width;
		specification.Height = m_Specification.Height;
		specification.Format = m_Specification.Format;
		specification.MipLevels = m_Specification.MipLevels;
		specification.Type = ImageType::Storage;
		storageImage->RT_Invalidate();
		storageImage->RT_UploadImageData(this);
		return storageImage;
	}

	bool DirectXImage2D::RT_ReadPixel(uint32_t x, uint32_t y, uint32_t& out_Pixel)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		if (x >= m_Specification.Width || y >= m_Specification.Height)
			return false;

		Ref<DirectXImage2D> sourceImage = (m_Specification.Type == ImageType::Storage) ? this : RT_GetStorageImage().As<DirectXImage2D>();

		auto ctx = DirectXRenderer::GetContext();
		D3D11_MAPPED_SUBRESOURCE ms;
		HRESULT hr = ctx->Map(sourceImage->m_Resource, 0, D3D11_MAP_READ, 0, &ms);
		DX11_VERIFY(hr);
		if (FAILED(hr))
			return false;

		uint32_t* data = (uint32_t*)ms.pData;
		uint32_t padding = ms.RowPitch / 4;
		out_Pixel = data[y * padding + x];

		ctx->Unmap(sourceImage->m_Resource, 0);

		return true;
	}

	void DirectXImage2D::UpdateResource(Buffer imageData)
	{
		SK_CORE_ASSERT(imageData);
		Ref<DirectXImage2D> instance = this;
		Buffer buffer = Buffer::Copy(imageData);
		Renderer::Submit([instance, buffer]() mutable
		{
			instance->RT_UpdateResource(buffer);
			buffer.Release();
		});
	}

	void DirectXImage2D::UpdateResource(Ref<DirectXImage2D> imageData)
	{
		Ref<DirectXImage2D> instance = this;
		Renderer::Submit([instance, imageData]()
		{
			instance->RT_UpdateResource(imageData);
		});
	}

	void DirectXImage2D::RT_UpdateResource(Buffer imageData)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		SK_CORE_VERIFY(imageData);

		SK_CORE_VERIFY(imageData.Size == ((uint64_t)m_Specification.Width * m_Specification.Height * utils::GetFormatDataSize(m_Specification.Format)));

		auto context = DirectXRenderer::GetContext();
		const uint32_t formatDataSize = utils::GetFormatDataSize(m_Specification.Format);
		context->UpdateSubresource(m_Resource, 0, nullptr, imageData.As<const void*>(), m_Specification.Width * formatDataSize, 0);
	}

	void DirectXImage2D::RT_UpdateResource(Ref<DirectXImage2D> imageData)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		SK_CORE_VERIFY(imageData);

		auto ctx = DirectXRenderer::GetContext();
		ctx->CopyResource(m_Resource, imageData->m_Resource);
	}

	void DirectXImage2D::RT_CreateView()
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
		viewDesc.Format = utils::ImageFormatToD3D11ForView(m_Specification.Format);
		viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipLevels = -1;
		viewDesc.Texture2D.MostDetailedMip = 0;

		auto device = DirectXRenderer::GetDevice();
		SK_DX11_CALL(device->CreateShaderResourceView(m_Resource, &viewDesc, &m_View));
	}

}
