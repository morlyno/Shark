#include "skpch.h"
#include "GPUTimer.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXGPUTimer.h"

namespace Shark {

	Ref<GPUTimer> GPUTimer::Create(const std::string& name)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No RendererAPI Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXGPUTimer>::Create(name);
		}
		SK_CORE_ASSERT(false, "Unkonw RendererAPI");
		return nullptr;
	}

}

