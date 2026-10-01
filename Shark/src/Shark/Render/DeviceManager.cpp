#include "skpch.h"
#include "DeviceManager.h"

#if SK_WITH_DX11
	#include "Shark/Platform/DirectX11/DirectX11DeviceManager.h"
#endif

#if SK_WITH_VULKAN
	#include "Shark/Platform/Vulkan/VulkanDeviceManager.h"
#endif

#include "Shark/Debug/Profiler.h"

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

	bool DeviceManager::CreateDevice(const DeviceSpecification& specification)
	{
		SK_PROFILE_FUNCTION();

		m_Specification = specification;

		if (!CreateInstanceInternal())
		{
			SK_CORE_ERROR_TAG(Log::Tag::Renderer, "Failed to create instance");
			return false;
		}

		if (!CreateDeviceInternal())
		{
			SK_CORE_ERROR_TAG(Log::Tag::Renderer, "Failed to create device");
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
		SK_PROFILE_FUNCTION();

		m_NvrhiDevice->runGarbageCollection();
		RunGarbageCollectionInternal();
	}

	uint64_t DeviceManager::ExecuteCommandList(nvrhi::ICommandList* commandList, nvrhi::CommandQueue queue)
	{
		SK_PROFILE_FUNCTION();

		return m_NvrhiDevice->executeCommandList(commandList, queue);
	}

	void DeviceManager::ExecuteCommand(std::move_only_function<void(nvrhi::ICommandList*)> cmd)
	{
		ExecuteCommand(nvrhi::CommandQueue::Graphics, std::move(cmd));
	}

	void DeviceManager::ExecuteCommand(nvrhi::CommandQueue queue, std::move_only_function<void(nvrhi::ICommandList*)> cmd)
	{
		SK_PROFILE_FUNCTION();

		auto commandList = GetTemporaryCommandList(queue);
		commandList->open();
		cmd(commandList);
		commandList->close();
		ExecuteCommandList(commandList, queue);
	}

	nvrhi::CommandListHandle DeviceManager::GetTemporaryCommandList(nvrhi::CommandQueue queue)
	{
		return GetOrCreateThreadLocalCommandList(queue);
	}

	nvrhi::CommandListHandle DeviceManager::GetOrCreateThreadLocalCommandList(nvrhi::CommandQueue queue)
	{
		SK_PROFILE_FUNCTION();

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

}
