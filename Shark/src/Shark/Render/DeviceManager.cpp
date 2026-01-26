#include "skpch.h"
#include "DeviceManager.h"

#include "Shark/Core/Application.h"

#if SK_WITH_DX11
#include "Shark/Platform/DirectX11/DirectX11DeviceManager.h"
#endif

namespace Shark {

	Scope<DeviceManager> DeviceManager::Create(nvrhi::GraphicsAPI api)
	{
		switch (api)
		{
#if SK_WITH_DX11
			case nvrhi::GraphicsAPI::D3D11: return Scope<DirectX11DeviceManager>::Create();
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

	DeviceManager::DeviceManager()
	{

	}

	bool DeviceManager::CreateDevice(const DeviceSpecification& specification)
	{
		m_Sepcification = specification;
		if (!CreateDeviceInternal())
			return false;

		if (m_Sepcification.EnableNvrhiValidationLayer)
		{
			m_NvrhiDevice = nvrhi::validation::createValidationLayer(m_NvrhiDevice);
		}

		m_CommandList = m_NvrhiDevice->createCommandList(
			nvrhi::CommandListParameters()
				.setEnableImmediateExecution(true)
				.setQueueType(nvrhi::CommandQueue::Graphics)
		);

		return true;
	}

	void DeviceManager::ExecuteCommandList(nvrhi::ICommandList* commandList)
	{
		m_NvrhiDevice->executeCommandList(commandList);
	}

	void DeviceManager::ExecuteCommandListLocked(nvrhi::ICommandList* commandList)
	{
		m_ExecutionMutex.lock();
		m_NvrhiDevice->executeCommandList(commandList);
		m_ExecutionMutex.unlock();
	}

}
