#include "skpch.h"
#include "SwapChain.h"

#include "Shark/Core/Application.h"
#include "Shark/Platform/DirectX11/DirectX11Swapchain.h"

namespace Shark {

	Ref<SwapChain> SwapChain::Create(const SwapChainSpecification& specification)
	{
		switch (Application::Get().GetDeviceManager()->GetGraphicsAPI())
		{
			case nvrhi::GraphicsAPI::D3D11:
				return Ref<DirectX11SwapChain>::Create(specification);
		}
		SK_CORE_VERIFY(false, "Unkown graphics api");
		return nullptr;
	}

}
