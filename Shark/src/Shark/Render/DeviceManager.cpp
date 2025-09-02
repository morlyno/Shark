#include "skpch.h"
#include "DeviceManager.h"

#if SK_WITH_DX11
#include "Shark/Platform/DirectX11/DirectX11DeviceManager.h"
#endif

namespace Shark {

	DeviceManager* DeviceManager::Create(nvrhi::GraphicsAPI api)
	{
		switch (api)
		{
#if SK_WITH_DX11
			case nvrhi::GraphicsAPI::D3D11: return sknew DirectX11DeviceManager();
#endif

#if SK_WITH_DX12
#error DirectX 12 is not implemented
			case nvrhi::GraphicsAPI::D3D12:
				break;
#endif

#if SK_WITH_VULKAN
#error Vulkan is not implemented
			case nvrhi::GraphicsAPI::VULKAN:
				break;
#endif
		}

		SK_CORE_VERIFY(false, "Unknown graphics api");
		return nullptr;
	}

	bool DeviceManager::CreateDevice(const DeviceSpecification& specification)
	{
		m_Sepcification = specification;
		return CreateDeviceInternal();
	}

}
