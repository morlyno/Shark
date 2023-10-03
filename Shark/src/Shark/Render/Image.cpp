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
			case ImageType::FrameBuffer: return "FrameBuffer";
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

	Ref<Image2D> Image2D::Create(const ImageSpecification& specs, Ref<Image2D> data)
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No Renderer API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXImage2D>::Create(specs, data);
		}
		SK_CORE_ASSERT(false, "Unkown Renderer API");
		return nullptr;
	}

}
