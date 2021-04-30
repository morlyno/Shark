#include "skpch.h"
#include "Texture.h"

#include "Platform/DirectX11/DirectXTexture.h"

namespace Shark {
	
	Ref<Texture2D> Texture2D::Create(const SamplerSpecification& specs, const std::string& filepath)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXTexture2D>::Create(specs, filepath);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const SamplerSpecification& specs, uint32_t width, uint32_t height, uint32_t flatcolor)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXTexture2D>::Create(specs, width, height, flatcolor);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const SamplerSpecification& specs, uint32_t width, uint32_t height, void* data)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXTexture2D>::Create(specs, width, height, data);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}
}