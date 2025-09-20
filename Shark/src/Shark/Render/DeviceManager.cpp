#include "skpch.h"
#include "DeviceManager.h"

#include "Shark/Core/Application.h"

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

	void DeviceManager::ExecuteCommandList(CommandList* commandList)
	{
		ExecuteCommandList(commandList->m_CommandList);
	}

	void DeviceManager::ExecuteCommandListLocked(CommandList* commandList)
	{
		ExecuteCommandListLocked(commandList->m_CommandList);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	////////// Auto Lockable CommandList //////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	CommandList::CommandList(DeviceManager* deviceManager, nvrhi::CommandListHandle commandList)
		: m_DeviceManager(deviceManager), m_CommandList(commandList)
	{
	}

	CommandList::~CommandList()
	{
	}

	void CommandList::open()
	{
		m_DeviceManager->LockCommandList(m_CommandList);
		m_CommandList->open();
	}

	void CommandList::close()
	{
		m_CommandList->close();
		m_DeviceManager->UnlockCommandList(m_CommandList);
	}

	void CommandList::writeTexture(nvrhi::ITexture* dest, uint32_t arraySlice, uint32_t mipLevel, const void* data, size_t rowPitch, size_t depthPitch)
	{
		m_CommandList->writeTexture(dest, arraySlice, mipLevel, data, rowPitch, depthPitch);
	}

	void CommandList::writeBuffer(nvrhi::IBuffer* b, const void* data, size_t dataSize, uint64_t destOffsetBytes)
	{
		m_CommandList->writeBuffer(b, data, dataSize, destOffsetBytes);
	}

}
