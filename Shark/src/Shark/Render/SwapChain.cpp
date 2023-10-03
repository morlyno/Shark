#include "skpch.h"
#include "SwapChain.h"

#include "Shark/Core/Window.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXSwapChain.h"

namespace Shark {

	Ref<SwapChain> SwapChain::Create(const SwapChainSpecifications& specs)
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No RendererAPI specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXSwapChain>::Create(specs);
		}
		SK_CORE_ASSERT(false, "Unkown Renderer API");
		return nullptr;
	}

}

