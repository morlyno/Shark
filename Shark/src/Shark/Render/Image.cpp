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

	Ref<Image2D> Image2D::Create(const ImageSpecification& specs)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No Renderer API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXImage2D>::Create(specs);
		}
		SK_CORE_ASSERT(false, "Unkown Renderer API");
		return nullptr;
	}

	Ref<Image2D> Image2D::Create(const ImageSpecification& specs, Buffer imageData)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No Renderer API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXImage2D>::Create(specs, imageData);
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

	Ref<Image2D> Image2D::Create(ImageFormat format, uint32_t width, uint32_t height, Buffer imageData)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No Renderer API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXImage2D>::Create(format, width, height, imageData);
		}
		SK_CORE_ASSERT(false, "Unkown Renderer API");
		return nullptr;
	}

	Ref<Image2D> Image2D::Create(Ref<TextureSource> source, uint32_t mipLeves)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No Renderer API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXImage2D>::Create(source, mipLeves);
		}
		SK_CORE_ASSERT(false, "Unkown Renderer API");
		return nullptr;
	}

	Ref<Image2D> Image2D::LoadFromDisc(const std::filesystem::path& filepath)
	{
		auto image = Image2D::Create();
		ImageSerializer serializer(image);
		if (serializer.Deserialize(filepath))
			return image;
		return nullptr;
	}

}
