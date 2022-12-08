#include "skpch.h"
#include "DirectXImage.h"

#include "Shark/Asset/ResourceManager.h"
#include "Shark/Serialization/TextureSerializers.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/DirectX11/DirectXSwapChain.h"

#include <stb_image.h>

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
				case ImageFormat::R32_SINT: return 4;
				case ImageFormat::Depth32: return 4;
			}
			SK_CORE_ASSERT(false, "Unkown ImageFormat");
			return 0;
		}

	}

	DirectXImage2D::DirectXImage2D()
	{
	}

	DirectXImage2D::DirectXImage2D(const ImageSpecification& specs)
		: m_Specification(specs)
	{
		CreateResource();
	}

	DirectXImage2D::DirectXImage2D(const ImageSpecification& specs, Buffer imageData)
		: m_Specification(specs)
	{
		CreateResource();
		UpdateResource(imageData);
	}

	DirectXImage2D::DirectXImage2D(const ImageSpecification& specs, Ref<Image2D> data)
		: m_Specification(specs)
	{
		CreateResource();
		UpdateResource(data.As<DirectXImage2D>());
	}

	DirectXImage2D::DirectXImage2D(ImageFormat format, uint32_t width, uint32_t height, Buffer imageData)
		: DirectXImage2D({ format, width, height }, imageData)
	{
	}

	DirectXImage2D::DirectXImage2D(const std::filesystem::path& filePath)
	{
		Set(filePath);
	}

	DirectXImage2D::DirectXImage2D(const ImageSpecification& specs, ID3D11Texture2D* resource, bool createView)
		: m_Specification(specs)
	{
		m_Resource = resource;
		m_Resource->AddRef();
		if (createView)
			CreateView();
	}

	DirectXImage2D::DirectXImage2D(Ref<DirectXSwapChain> swapchain, bool createView)
	{
		auto& scSpec = swapchain->GetSpecification();
		m_Specification.Width = scSpec.Widht;
		m_Specification.Height = scSpec.Height;
		m_Specification.Type = ImageType::FrameBuffer;
		m_Specification.Format = ImageFormat::RGBA8;

		Ref<DirectXImage2D> instance = this;
		Renderer::Submit([instance, swapchain, createView]()
		{
			IDXGISwapChain* dxSwapchain = swapchain->GetSwapChain();
			ID3D11Texture2D* texture = nullptr;
			SK_DX11_CALL(dxSwapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&texture));
			instance->m_Resource = texture;

			if (createView)
				instance->RT_CreateView();
		});

	}

	DirectXImage2D::~DirectXImage2D()
	{
		Release();
	}

	void DirectXImage2D::Release()
	{
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

	void DirectXImage2D::Set(const ImageSpecification& specs, Buffer imageData)
	{
		Release();

		m_Specification = specs;
		CreateResource();
		UpdateResource(imageData);
	}

	void DirectXImage2D::Set(const ImageSpecification& specs, Ref<Image2D> data)
	{
		SK_CORE_VERIFY(this != data.Raw());

		Release();

		m_Specification = specs;

		CreateResource();
		UpdateResource(data.As<DirectXImage2D>());
	}

	void DirectXImage2D::Set(const std::filesystem::path& filePath)
	{
		ImageFormat format;
		int width, height, components;
		Buffer imagedata;

		Buffer filedata = FileSystem::ReadBinary(filePath);
		imagedata.Data = stbi_load_from_memory(filedata.As<stbi_uc>(), (int)filedata.Size, &width, &height, &components, STBI_rgb_alpha);
		filedata.Release();

		SK_CORE_ASSERT(imagedata.Data);
		if (!imagedata.Data)
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to load Image from disc! {}", filePath);
			SK_CORE_WARN_TAG("Renderer", stbi_failure_reason());
			return;
		}

		imagedata.Size = (uint64_t)width * height * components;
		format = ImageFormat::RGBA8;

		m_Specification.Width = width;
		m_Specification.Height = height;
		m_Specification.Format = ImageFormat::RGBA8;
		m_Specification.MipLevels = 0;
		m_Specification.Type = ImageType::Texture;

		Ref<DirectXImage2D> instance = this;
		Renderer::Submit([instance, imagedata]() mutable
		{
			instance->RT_CreateResource();
			instance->RT_UpdateResource(imagedata);
			imagedata.Release();

			DirectXRenderer::Get()->RT_GenerateMips(instance);
		});
	}

	void DirectXImage2D::ReloadFromDisc()
	{
		SK_CORE_VERIFY(!m_FilePath.empty());
		Set(m_FilePath);
	}

	void DirectXImage2D::Resize(uint32_t width, uint32_t height)
	{
		if (m_Specification.Width == width && m_Specification.Height == height)
			return;

		m_Specification.Width = width;
		m_Specification.Height = height;

		Release();
		CreateResource();
		// UpdateResource();
	}

	bool DirectXImage2D::CopyTo(Ref<Image2D> image)
	{
		const auto& destSpecs = image->GetSpecification();
		if (!IsImageCompadible(destSpecs))
		{
			SK_CORE_WARN("Unable to Copy Resource!");
			return false;
		}

		Ref<DirectXImage2D> instance = this;
		Renderer::Submit([instance, dxImage = image.As<DirectXImage2D>()]()
		{
			instance->RT_UpdateResource(dxImage);
		});

		return true;
	}

	bool DirectXImage2D::ReadPixel(uint32_t x, uint32_t y, uint32_t& out_Pixel)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		if (m_Specification.Type != ImageType::Storage)
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

	void DirectXImage2D::RT_CopyTo(Ref<Image2D> image)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		auto dxImage = image.As<DirectXImage2D>();
		dxImage->RT_UpdateResource(this);
	}

	Buffer DirectXImage2D::LoadDataFromFile(const std::filesystem::path& filePath)
	{
		AssetHandle sourceHandle = ResourceManager::GetAssetHandleFromFilePath(filePath);
		auto source = ResourceManager::GetAsset<TextureSource>(sourceHandle);
		if (source)
		{
			m_Specification.Format = source->Format;
			m_Specification.Width = source->Width;
			m_Specification.Height = source->Height;
			m_Specification.MipLevels = 0;
			return source->ImageData;
		}

		return Buffer{};
	}

	void DirectXImage2D::CreateResource()
	{
		Ref<DirectXImage2D> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_CreateResource();
		});
	}

	void DirectXImage2D::UpdateResource(Buffer imageData)
	{
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

	void DirectXImage2D::CreateView()
	{
		Ref<DirectXImage2D> instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_CreateView();
		});
	}

	void DirectXImage2D::RT_CreateResource()
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		auto device = DirectXRenderer::GetDevice();

		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = m_Specification.Width;
		desc.Height = m_Specification.Height;
		desc.MipLevels = m_Specification.MipLevels;
		desc.ArraySize = 1;
		desc.Format = utils::ImageFormatToD3D11ForResource(m_Specification.Format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;

		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		switch (m_Specification.Type)
		{
			case ImageType::Texture:
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				break;
			case ImageType::Storage:
				desc.Usage = D3D11_USAGE_STAGING;
				desc.BindFlags = 0;
				desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
				break;
			case ImageType::FrameBuffer:
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				desc.BindFlags |= IsDepthImage() ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;
				break;
		}

		SK_DX11_CALL(device->CreateTexture2D(&desc, nullptr, &m_Resource));
		if (m_Specification.Type != ImageType::Storage)
			RT_CreateView();
	}

	void DirectXImage2D::RT_UpdateResource(Buffer imageData)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		SK_CORE_VERIFY(imageData);

		auto context = DirectXRenderer::GetContext();
		context->UpdateSubresource(m_Resource, 0, nullptr, imageData.As<const void*>(), m_Specification.Width * 4, 0);
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
