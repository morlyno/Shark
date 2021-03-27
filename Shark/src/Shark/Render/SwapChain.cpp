#include "skpch.h"
#include "SwapChain.h"

#include "Shark/Render/RendererCommand.h"

namespace Shark {

	Ref<SwapChain> SwapChain::Create(const SwapChainSpecifications& specs)
	{
		return RendererCommand::CreateSwapChain(specs);
	}

}