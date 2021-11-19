#include "skpch.h"
#include "Texture.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXTexture.h"

namespace Shark {

	Ref<Texture2D> Texture2D::Create(Ref<Image2D> image, const SamplerProps& props)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXTexture2D>::Create(image, props);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const std::filesystem::path& filepath, const SamplerProps& props)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXTexture2D>::Create(filepath, props);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height, void* data, const SamplerProps& props)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXTexture2D>::Create(width, height, data, props);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	Ref<Texture2DArray> Texture2DArray::Create(uint32_t count, uint32_t startOffset)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXTexture2DArray>::Create(count, startOffset);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

}