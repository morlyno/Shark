#include "skpch.h"
#include "Image.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXImage.h"

namespace Shark {

	Ref<Image2D> Image2D::Create()
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No Renderer API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXImage2D>::Create();
		}
		SK_CORE_ASSERT(false, "Unkown Renderer API");
		return nullptr;
	}

	Ref<Image2D> Image2D::Create(const ImageSpecification& specs)
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No Renderer API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXImage2D>::Create(specs);
		}
		SK_CORE_ASSERT(false, "Unkown Renderer API");
		return nullptr;
	}

	Ref<ImageView> ImageView::Create(Ref<Image2D> image, uint32_t mipSlice)
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No Renderer API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXImageView>::Create(image, mipSlice);
		}
		SK_CORE_ASSERT(false, "Unkown Renderer API");
		return nullptr;
	}

	namespace ImageUtils {

		uint32_t CalcMipLevels(uint32_t widht, uint32_t height)
		{
			return (uint32_t)glm::floor(glm::log2((float)glm::max(widht, height))) + 1;
		}

		bool IsDepthFormat(Shark::ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None:
				case ImageFormat::RGBA8UNorm:
				case ImageFormat::RGBA16Float:
				case ImageFormat::RGBA32Float:
				case ImageFormat::R8UNorm:
				case ImageFormat::R32SINT:
				case ImageFormat::RG16SNorm:
					return false;

				case ImageFormat::Depth32:
				case ImageFormat::Depth24UNormStencil8UINT:
					return true;

				default:
					SK_CORE_VERIFY(false, "Unkown ImageFormat");
					break;
			}

			SK_CORE_ASSERT(false, "Unkown ImageFormat", (uint16_t)format);
			return false;
		}

		bool IsIntegerBased(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None:
				case ImageFormat::RGBA8UNorm:
				case ImageFormat::RGBA16Float:
				case ImageFormat::RGBA32Float:
				case ImageFormat::R8UNorm:
				case ImageFormat::RG16SNorm:
				case ImageFormat::Depth32:
				case ImageFormat::Depth24UNormStencil8UINT:
					return false;

				case ImageFormat::R32SINT:
					return true;

				default:
					SK_CORE_VERIFY(false, "Unkown ImageFormat");
					break;
			}

			SK_CORE_ASSERT(false, "Unkown ImageFormat", (uint16_t)format);
			return false;
		}

		uint32_t GetFormatBPP(ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None: return 0;
				case ImageFormat::RGBA8UNorm: return 4;
				case ImageFormat::RGBA16Float: return 8; // 4 * 2bytes
				case ImageFormat::RGBA32Float: return 4 * 4; // 4 * 4bytes
				case ImageFormat::R8UNorm: return 1;
				case ImageFormat::R32SINT: return 4;
				case ImageFormat::Depth32: return 4;
			}
			SK_CORE_ASSERT(false, "Unkown ImageFormat");
			return 0;
		}

	}

}
