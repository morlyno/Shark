#include "skpch.h"
#include "Image.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXImage.h"

namespace Shark {

	std::string ImageFormatToString(ImageFormat format)
	{
		switch (format)
		{
			case ImageFormat::None: return "None";
			case ImageFormat::RGBA8: return "RGBA8";
			case ImageFormat::R32_SINT: return "R32_SINT";
			case ImageFormat::Depth32: return "Depth32";
			case ImageFormat::SwapChain: return "(SwapChain) [SOON DEPRECATED]";
		}
		SK_CORE_ASSERT(false);
		return "Unkonw";
	}

	std::string ImageTypeToString(ImageType usage)
	{
		switch (usage)
		{
			case ImageType::Default: return "Default";
			case ImageType::Immutable: return "Immutable";
			case ImageType::Dynamic: return "Dynamic";
			case ImageType::Staging: return "Staging";
		}
		SK_CORE_ASSERT(false);
		return "Unkonw";
	}

	std::string ImageUsageToString(uint32_t flags)
	{
		if (flags == ImageUsageNone)
			return "None";

		std::string str;

		if (flags & ImageUsageTexture)
			str.empty() ? str = "ShaderResource" : str += " | ShaderResource";

		if (flags & ImageUsageFrameBuffer)
			str.empty() ? str = "FrameBuffer" : str += " | FrameBuffer";

		if (flags & ImageUsageDethStencil)
			str.empty() ? str = "DethStencil" : str += " | DethStencil";

		return str;
	}


	Ref<Image2D> Image2D::Create(const ImageSpecification& specs)
	{
		return Create(specs, nullptr);
	}

	Ref<Image2D> Image2D::Create(const ImageSpecification& specs, void* data)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No RendererAPI specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXImage2D>::Create(data, specs);
		}
		SK_CORE_ASSERT(false, "Unkonw RendererAPI");
		return nullptr;
	}

	Ref<Image2D> Image2D::Create(const std::filesystem::path& filepath, const ImageSpecification& specs)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No RendererAPI specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXImage2D>::Create(filepath, specs);
		}
		SK_CORE_ASSERT(false, "Unkonw RendererAPI");
		return nullptr;
	}

}
