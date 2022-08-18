#include "skpch.h"
#include "Image.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXImage.h"

namespace Shark {

	std::string EnumToString(ImageFormat format)
	{
		switch (format)
		{
			case ImageFormat::None: return "None";
			case ImageFormat::RGBA8: return "RGBA8";
			case ImageFormat::R32_SINT: return "R32_SINT";
			case ImageFormat::Depth32: return "Depth32";
		}
		SK_CORE_ASSERT(false);
		return "Unkonw";
	}

	Ref<Image2D> Image2D::Create()
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No Renderer API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXImage2D>::Create();
		}
		SK_CORE_ASSERT(false, "Unkown Renderer API");
		return nullptr;
	}

	Ref<Image2D> Image2D::Create(const ImageSpecification& specs, void* data)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No Renderer API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXImage2D>::Create(specs, data);
		}
		SK_CORE_ASSERT(false, "Unkown Renderer API");
		return nullptr;
	}

	Ref<Image2D> Image2D::Create(ImageFormat format, uint32_t width, uint32_t height, void* data)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No Renderer API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXImage2D>::Create(format, width, height, data);
		}
		SK_CORE_ASSERT(false, "Unkown Renderer API");
		return nullptr;
	}

	Ref<Image2D> Image2D::Create(const ImageSpecification& specs, Ref<Image2D> data)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No Renderer API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXImage2D>::Create(specs, data);
		}
		SK_CORE_ASSERT(false, "Unkown Renderer API");
		return nullptr;
	}

	Ref<Image2D> Image2D::Create(const std::filesystem::path& filePath)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No Renderer API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXImage2D>::Create(filePath);
		}
		SK_CORE_ASSERT(false, "Unkown Renderer API");
		return nullptr;
	}

}
