#include "skpch.h"
#include "DirectXImage.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Texture.h"
#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXContext.h"
#include "Platform/DirectX11/DirectXRenderer.h"

#include "Shark/Debug/Profiler.h"
#include <stb_image.h>

namespace Shark {

	namespace DXImageUtils {

		DXGI_FORMAT ImageFormatToDXGI(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None: return DXGI_FORMAT_UNKNOWN;
				case ImageFormat::RGBA8UNorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
				case ImageFormat::RGBA16Float: return DXGI_FORMAT_R16G16B16A16_FLOAT;
				case ImageFormat::RGBA32Float: return DXGI_FORMAT_R32G32B32A32_FLOAT;
				case ImageFormat::R8UNorm: return DXGI_FORMAT_R8_UNORM;
				case ImageFormat::R32SINT: return DXGI_FORMAT_R32_SINT;
				case ImageFormat::RG16Float: return DXGI_FORMAT_R16G16_FLOAT;
				case ImageFormat::RG16SNorm: return DXGI_FORMAT_R16G16_SNORM;
				case ImageFormat::Depth32: return DXGI_FORMAT_D32_FLOAT;
				case ImageFormat::Depth24UNormStencil8UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
			}

			SK_CORE_ASSERT(false, "Unkown Image Format");
			return DXGI_FORMAT_UNKNOWN;
		}

		DXGI_FORMAT FixImageFormatForResource(DXGI_FORMAT format)
		{
			switch (format)
			{
				case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_R32_TYPELESS;
				case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24G8_TYPELESS;
			}

			return format;
		}

		DXGI_FORMAT FixImageFormatForView(DXGI_FORMAT format)
		{
			switch (format)
			{
				case DXGI_FORMAT_D32_FLOAT: return DXGI_FORMAT_R32_FLOAT;
				case DXGI_FORMAT_D24_UNORM_S8_UINT: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			}

			return format;
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
			return fmt::format("{} - {} ({}, {}) Mips: {}", specification.Type, specification.Format, specification.Width, specification.Height, specification.MipLevels);
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
		SK_CORE_VERIFY(m_Info.Sampler ? m_Info.Resource || m_Info.View : true);
		if (!m_Info.Resource)
			return;

		Renderer::SubmitResourceFree([info = m_Info, uavs = m_PerMipUAVs]()
		{
			DirectXAPI::ReleaseObject(info.Resource);
			DirectXAPI::ReleaseObject(info.View);
			DirectXAPI::ReleaseObject(info.Sampler);

			for (const auto [mip, uav] : uavs)
				DirectXAPI::ReleaseObject(uav);
		});

		m_Info.Resource = nullptr;
		m_Info.View = nullptr;
		m_Info.Sampler = nullptr;
		m_PerMipUAVs.clear();
	}

	void DirectXImage2D::Invalidate()
	{
		if (m_Specification.MipLevels == 0)
			m_Specification.MipLevels = ImageUtils::CalcMipLevels(m_Specification.Width, m_Specification.Height);

		Ref<DirectXImage2D> instance = this;
		Renderer::Submit([instance]() { instance->RT_Invalidate(); });
	}

	void DirectXImage2D::RT_Invalidate()
	{
		SK_PROFILE_FUNCTION();
		Release();

		if (m_Specification.MipLevels == 0)
			m_Specification.MipLevels = ImageUtils::CalcMipLevels(m_Specification.Width, m_Specification.Height);

		if (m_Specification.DebugName.empty())
			m_Specification.DebugName = utils::GenerateDebugName(m_Specification);

		DXGI_FORMAT dxgiFormat = DXImageUtils::ImageFormatToDXGI(m_Specification.Format);

		D3D11_TEXTURE2D_DESC texture2dDesc{};
		texture2dDesc.Width = m_Specification.Width;
		texture2dDesc.Height = m_Specification.Height;
		texture2dDesc.Format = DXImageUtils::FixImageFormatForResource(dxgiFormat);
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

		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		DirectXAPI::CreateTexture2D(dxDevice, texture2dDesc, nullptr, m_Info.Resource);
		DirectXAPI::SetDebugName(m_Info.Resource, m_Specification.DebugName);

		if (m_Specification.Type != ImageType::Storage)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Format = DXImageUtils::FixImageFormatForView(dxgiFormat);
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
			DirectXAPI::CreateShaderResourceView(dxDevice, m_Info.Resource, shaderResourceViewDesc, m_Info.View);
			DirectXAPI::SetDebugName(m_Info.View, m_Specification.DebugName);
		}

		if (m_Specification.CreateSampler)
		{
			D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(D3D11_DEFAULT);
			samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			DirectXAPI::CreateSamplerState(dxDevice, samplerDesc, m_Info.Sampler);
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
		SK_PROFILE_FUNCTION();

		const uint64_t memoryUsage = (uint64_t)m_Specification.Width * m_Specification.Height * m_Specification.Layers * ImageUtils::GetFormatBPP(m_Specification.Format);
		SK_CORE_VERIFY(imageData.Size == memoryUsage, "{} == {}", imageData.Size, memoryUsage);

		auto device = DirectXContext::GetCurrentDevice();
		const uint32_t formatDataSize = ImageUtils::GetFormatBPP(m_Specification.Format);

		//device->UpdateSubresource(m_Info.Resource, 0, nullptr, imageData.As<const void*>(), m_Specification.Width * formatDataSize, 0);

		auto cmd = device->AllocateCommandBuffer();
		cmd->UpdateSubresource(m_Info.Resource, 0, nullptr, imageData.As<const void*>(), m_Specification.Width * formatDataSize, 0);
		device->FlushCommandBuffer(cmd);
	}

	bool DirectXImage2D::RT_ReadPixel(uint32_t x, uint32_t y, uint32_t& out_Pixel)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());
		SK_CORE_VERIFY(m_Specification.Type == ImageType::Storage);

		if (x >= m_Specification.Width || y >= m_Specification.Height)
			return false;

		auto device = DirectXContext::GetCurrentDevice();

		D3D11_MAPPED_SUBRESOURCE mappedMemory;
		device->MapMemory(m_Info.Resource, 0, D3D11_MAP_READ, mappedMemory);

		uint32_t* data = (uint32_t*)mappedMemory.pData;
		uint32_t padding = mappedMemory.RowPitch / 4;
		out_Pixel = data[y * padding + x];

		device->UnmapMemory(m_Info.Resource, 0);

		return true;
	}

	void DirectXImage2D::RT_CopyToHostBuffer(Buffer& buffer)
	{
		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		DXGI_FORMAT dxgiImageFormat = DXImageUtils::ImageFormatToDXGI(m_Specification.Format);

		D3D11_TEXTURE2D_DESC textureDesc{};
		textureDesc.Width = m_Specification.Width;
		textureDesc.Height = m_Specification.Height;
		textureDesc.Format = DXImageUtils::FixImageFormatForResource(dxgiImageFormat);
		textureDesc.MipLevels = m_Specification.MipLevels;
		textureDesc.ArraySize = m_Specification.Layers;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_STAGING;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		ID3D11Texture2D* staging;
		DirectXAPI::CreateTexture2D(dxDevice, textureDesc, nullptr, staging);

		uint32_t mipCount = 1;
		for (uint32_t mip = 0; mip < mipCount; mip++)
		{
			uint32_t subresource = D3D11CalcSubresource(mip, 0, mipCount); 
			device->CopySubresource(staging, subresource, 0, 0, 0, m_Info.Resource, subresource, nullptr);
		}

		void* mappedMemory;
		device->MapMemory(staging, 0, D3D11_MAP_READ, mappedMemory);
		uint64_t bufferSize = m_Specification.Width * m_Specification.Height * ImageUtils::GetFormatBPP(m_Specification.Format);
		buffer.Allocate(bufferSize);
		memcpy(buffer.Data, mappedMemory, bufferSize);
		device->UnmapMemory(staging, 0);

		staging->Release();
	}

	void DirectXImage2D::RT_CreateUnorderAccessView(uint32_t mipSlice)
	{
		if (m_PerMipUAVs.contains(mipSlice) && m_PerMipUAVs.at(mipSlice))
			return;

		DXGI_FORMAT dxgiImageFormat = DXImageUtils::ImageFormatToDXGI(m_Specification.Format);

		D3D11_UNORDERED_ACCESS_VIEW_DESC unorderedAccessViewDesc{};
		unorderedAccessViewDesc.Format = DXImageUtils::FixImageFormatForView(dxgiImageFormat);;
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

		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		ID3D11UnorderedAccessView* uav;
		DirectXAPI::CreateUnorderedAccessView(dxDevice, m_Info.Resource, unorderedAccessViewDesc, uav);
		std::string debugName = fmt::format("{} UAV (Mip={})", m_Specification.DebugName, mipSlice);
		DirectXAPI::SetDebugName(uav, debugName);

		m_PerMipUAVs[mipSlice] = uav;
	}

	void DirectXImage2D::RT_CreatePerMipUAV()
	{
		for (uint32_t i = 0; i < m_Specification.MipLevels; i++)
		{
			RT_CreateUnorderAccessView(i);
		}
	}

	DirectXImageView::DirectXImageView(Ref<Image2D> image, uint32_t mipSlice)
		: m_Image(image), m_MipSlice(mipSlice)
	{
		Invalidate();
	}

	DirectXImageView::~DirectXImageView()
	{
		if (!m_Info.View)
			return;

		Renderer::SubmitResourceFree([info = m_Info]()
		{
			DirectXAPI::ReleaseObject(info.Resource);
			DirectXAPI::ReleaseObject(info.View);
			DirectXAPI::ReleaseObject(info.Sampler);
		});

		m_Info.Resource = nullptr;
		m_Info.View = nullptr;
		m_Info.Sampler = nullptr;
	}

	void DirectXImageView::Invalidate()
	{
		Ref<DirectXImage2D> dxImage = m_Image.As<DirectXImage2D>();
		const auto& imageSpecification = dxImage->GetSpecification();
		SK_CORE_VERIFY(imageSpecification.MipLevels != 0);

		DXGI_FORMAT dxgiImageFormat = DXImageUtils::ImageFormatToDXGI(dxImage->GetSpecification().Format);

		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
		shaderResourceViewDesc.Format = DXImageUtils::FixImageFormatForView(dxgiImageFormat);

		if (dxImage->GetType() == ImageType::TextureCube)
		{
			shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			shaderResourceViewDesc.TextureCube.MipLevels = 1;
			shaderResourceViewDesc.TextureCube.MostDetailedMip = m_MipSlice;
		}
		else if (dxImage->GetSpecification().Layers > 1)
		{
			shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			shaderResourceViewDesc.Texture2DArray.ArraySize = dxImage->GetSpecification().Layers;
			shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
			shaderResourceViewDesc.Texture2DArray.MipLevels = 1;
			shaderResourceViewDesc.Texture2DArray.MostDetailedMip = m_MipSlice;
		}
		else
		{
			shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			shaderResourceViewDesc.Texture2D.MipLevels = 1;
			shaderResourceViewDesc.Texture2D.MostDetailedMip = m_MipSlice;
		}

		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		const auto& imageInfo = dxImage->GetDirectXImageInfo();
		m_Info.Resource = imageInfo.Resource;
		m_Info.Sampler = imageInfo.Sampler;
		m_Info.Resource->AddRef();
		m_Info.Sampler->AddRef();

		DirectXAPI::CreateShaderResourceView(dxDevice, dxImage->GetDirectXImageInfo().Resource, shaderResourceViewDesc, m_Info.View);
		m_DebugName = fmt::format("{} (Mip: {})", dxImage->GetSpecification().DebugName, m_MipSlice);
		DirectXAPI::SetDebugName(m_Info.View, m_DebugName);
	}

}
