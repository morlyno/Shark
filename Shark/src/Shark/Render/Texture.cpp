#include "skpch.h"
#include "Texture.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXTexture.h"

namespace Shark {

	std::string ToString(FilterMode filterMode)
	{
		switch (filterMode)
		{
			case FilterMode::Nearest: return "Nearest";
			case FilterMode::Linear:  return "Linear";
		}
		SK_CORE_ASSERT(false, "Unkown FilterMode");
		return "Unkown";
	}

	std::string ToString(WrapMode wrapMode)
	{
		switch (wrapMode)
		{
			case WrapMode::Repeat: return "Repeat";
			case WrapMode::Clamp:  return "Clamp";
			case WrapMode::Mirror: return "Mirror";
			case WrapMode::Border: return "Border";
		}
		SK_CORE_ASSERT(false, "Unkown FilterMode");
		return "Unkown";
	}

	FilterMode StringToFilterMode(std::string_view filterMode)
	{
		if (filterMode == "Nearest") return FilterMode::Nearest;
		if (filterMode == "Linear") return FilterMode::Linear;

		SK_CORE_ASSERT(false, "Unkown FilterMode");
		return FilterMode::Linear;
	}

	WrapMode StringToWrapMode(std::string_view wrapMode)
	{
		if (wrapMode == "Repeat") return WrapMode::Repeat;
		if (wrapMode == "Clamp") return WrapMode::Clamp;
		if (wrapMode == "Mirror") return WrapMode::Mirror;
		if (wrapMode == "Border") return WrapMode::Border;

		SK_CORE_ASSERT(false, "Unkown wrap mode");
		return WrapMode::Repeat;
	}


	Ref<Texture2D> Texture2D::Create()
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXTexture2D>::Create();
		}
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specs, Buffer imageData)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXTexture2D>::Create(specs, imageData);
		}
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}


	Ref<Texture2D> Texture2D::Create(ImageFormat format, uint32_t width, uint32_t height, Buffer imageData)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXTexture2D>::Create(format, width, height, imageData);
		}
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::filesystem::path& filePath)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXTexture2D>::Create(filePath);
		}
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specs, Ref<Texture2D> data)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXTexture2D>::Create(specs, data);
		}
		SK_CORE_ASSERT(false, "Unkown API");
		return nullptr;
	}

	Ref<Texture2DArray> Texture2DArray::Create(uint32_t count, uint32_t startOffset)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXTexture2DArray>::Create(count, startOffset);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

}