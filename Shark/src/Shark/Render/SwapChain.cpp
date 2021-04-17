#include "skpch.h"
#include "SwapChain.h"

#include "Platform/DirectX11/DirectXSwapChain.h"

namespace Shark {

	Ref<SwapChain> SwapChain::Create(const SwapChainSpecifications& specs)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXSwapChain>::Allocate(specs);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

}