#include "skpch.h"
#include "Viewport.h"

#include "Platform/DirectX11/DirectXViewport.h"

namespace Shark {

	Ref<Viewport> Viewport::Create(uint32_t width, uint32_t height)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXViewport>::Allocate(width, height);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

}