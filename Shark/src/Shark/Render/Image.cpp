#include "skpch.h"
#include "Image.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Serialization/TextureSerializers.h"
#include "Platform/DirectX11/DirectXImage.h"

namespace Shark {

	std::string ToString(ImageFormat format)
	{
		switch (format)
		{
			case ImageFormat::None: return "None";
			case ImageFormat::RGBA8: return "RGBA8";
			case ImageFormat::RGBA16F: return "RGBA16F";
			case ImageFormat::R32_SINT: return "R32_SINT";
			case ImageFormat::Depth32: return "Depth32";
			case ImageFormat::RGB32F: return "RGB32F";
			case ImageFormat::RGBA32F: return "RGBA32F";
			case ImageFormat::R8: return "R8";
			case ImageFormat::R16F: return "R16F";
		}

		SK_CORE_ASSERT(false, "Unkown ImageFormat");
		return "Unkown";
	}

	std::string ToString(ImageType type)
	{
		switch (type)
		{
			case ImageType::Texture: return "Texture";
			case ImageType::Storage: return "Storage";
			case ImageType::Atachment: return "FrameBuffer";
		}

		SK_CORE_ASSERT(false, "Unkown ImageType");
		return "Unkown";
	}

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
			return glm::floor(glm::log2((float)glm::max(widht, height))) + 1;
		}

		bool IsDepthFormat(Shark::ImageFormat format)
		{
			switch (format)
			{
				case ImageFormat::None:
				case ImageFormat::RGBA8:
				case ImageFormat::RGBA16F:
				case ImageFormat::RGB32F:
				case ImageFormat::RGBA32F:
				case ImageFormat::R8:
				case ImageFormat::R16F:
				case ImageFormat::R32_SINT:
					return false;

				case ImageFormat::Depth32:
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
				case ImageFormat::RGBA8:
				case ImageFormat::RGBA16F:
				case ImageFormat::RGB32F:
				case ImageFormat::RGBA32F:
				case ImageFormat::R8:
				case ImageFormat::R16F:
				case ImageFormat::Depth32:
					return false;

				case ImageFormat::R32_SINT:
					return true;

				default:
					SK_CORE_VERIFY(false, "Unkown ImageFormat");
					break;
			}

			SK_CORE_ASSERT(false, "Unkown ImageFormat", (uint16_t)format);
			return false;
		}

		uint32_t GetFormatDataSize(ImageFormat format)
		{
			switch (format)
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

	}

}
