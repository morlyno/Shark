#include "skpch.h"
#include "GPUTimer.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXGPUTimer.h"

namespace Shark {

	Ref<GPUTimer> GPUTimer::Create(const std::string& name)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No RendererAPI Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXGPUTimer>::Create(name);
		}
		SK_CORE_ASSERT(false, "Unkonw RendererAPI");
		return nullptr;
	}

}

