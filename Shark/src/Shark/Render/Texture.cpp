#include "skpch.h"
#include "Texture.h"

#include "RendererAPI.h"
#include "Platform/DirectX11/DirectXTexture.h"

namespace Shark {
	
	Ref<Texture2D> Texture2D::Create(const SamplerSpecification& specs, const std::string& filepath)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "RendererAPI not specified"); return nullptr;
			case RendererAPI::API::DirectX11: return CreateRef<DirectXTexture2D>(specs, filepath);
		}

		SK_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const SamplerSpecification& specs, uint32_t width, uint32_t height, uint32_t color, const std::string& name)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "RendererAPI not specified"); return nullptr;
			case RendererAPI::API::DirectX11: return CreateRef<DirectXTexture2D>(specs, width, height, color, name);
		}

		SK_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}