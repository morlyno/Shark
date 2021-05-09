#include "skpch.h"
#include "Shaders.h"

#include "Platform/DirectX11/DirectXShaders.h"

namespace Shark {

	Ref<Shaders> Shaders::Create(const std::string& filepath)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXShaders>::Create(filepath);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

}