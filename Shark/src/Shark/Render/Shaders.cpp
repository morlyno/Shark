#include "skpch.h"
#include "Shaders.h"
#include "RendererAPI.h"
#include "Platform/DirectX11/DirectXShaders.h"

namespace Shark {

	Ref<Shaders> Shaders::Create(const std::string& filepath)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "RendererAPI not specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Create_Ref<DirectXShaders>(filepath);
		}
		SK_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Shaders> Shaders::Create(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "RendererAPI not specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Create_Ref<DirectXShaders>(vertexshaderSrc, pixelshaderSrc);
		}
		SK_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
	
}