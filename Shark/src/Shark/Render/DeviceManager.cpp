#include "skpch.h"
#include "DeviceManager.h"

#include "Shark/Core/Application.h"

#if SK_WITH_DX11
	#include "Shark/Platform/DirectX11/DirectX11DeviceManager.h"
#endif

#if SK_WITH_VULKAN
	#include "Shark/Platform/Vulkan/VulkanDeviceManager.h"
#endif

#include <nvrhi/validation.h>

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
			case nvrhi::GraphicsAPI::VULKAN: return Scope<VulkanDeviceManager>::Create();
#endif
		}

		SK_CORE_VERIFY(false, "Unknown graphics api");
		return nullptr;
	}

	DeviceManager::DeviceManager()
	{

	}

	DeviceManager::~DeviceManager()
	{
		m_CommandLists.clear();

		DestroyInternal();
	}

	nvrhi::CommandListHandle DeviceManager::GetTemporaryCommandList(nvrhi::CommandQueue queue)
	{
		return GetOrCreateThreadLocalCommandList(queue);
	}

	nvrhi::CommandListHandle DeviceManager::GetOrCreateThreadLocalCommandList(nvrhi::CommandQueue queue)
	{
		auto threadID = std::this_thread::get_id();

		{
			std::shared_lock lock(m_CommandListMutex);
			const auto i = m_CommandLists.find(threadID);

			if (i != m_CommandLists.end() && i->second[queue])
				return i->second[queue];
		}

		std::scoped_lock lock(m_CommandListMutex);

		nvrhi::CommandListParameters params;
		params.queueType = queue;
		params.enableImmediateExecution = false;

		auto commandList = m_NvrhiDevice->createCommandList(params);

		m_CommandLists[threadID][queue] = commandList;
		return commandList;
	}

	bool DeviceManager::CreateDevice(const DeviceSpecification& specification)
	{
		m_Specification = specification;

		if (!CreateInstanceInternal())
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to create instance");
			return false;
		}

		if (!CreateDeviceInternal())
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to create device");
			return false;
		}

		if (m_Specification.EnableNvrhiValidationLayer)
		{
			m_NvrhiDevice = nvrhi::validation::createValidationLayer(m_NvrhiDevice);
		}

		return true;
	}

	void DeviceManager::RunGarbageCollection()
	{
		m_NvrhiDevice->runGarbageCollection();
		RunGarbageCollectionInternal();
	}

	void DeviceManager::ExecuteCommandList(nvrhi::ICommandList* commandList)
	{
		m_NvrhiDevice->executeCommandList(commandList);
	}

	void DeviceManager::ExecuteCommandListLocked(nvrhi::ICommandList* commandList)
	{
		LockQueue();
		m_NvrhiDevice->executeCommandList(commandList);
		UnlockQueue();
	}

}
