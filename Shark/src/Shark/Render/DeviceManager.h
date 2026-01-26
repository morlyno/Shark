#pragma once

#include "Shark/Core/Base.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	struct DeviceSpecification
	{
		int AdapterIndex = -1;
		uint32_t SwapchainBufferCount = 3;
		uint32_t MaxFramesInFlight = 2;

		bool AllowModeSwitch = false;
		bool EnableDebugRuntime = false;
		bool EnableNvrhiValidationLayer = false;
	};

	class DeviceManager
	{
	public:
		static Scope<DeviceManager> Create(nvrhi::GraphicsAPI api);
		bool CreateDevice(const DeviceSpecification& specification);

	public:
		virtual nvrhi::IDevice* GetDevice() const = 0;
		virtual nvrhi::GraphicsAPI GetGraphicsAPI() const = 0;
		const DeviceSpecification& GetSpecification() const { return m_Sepcification; }

		void ExecuteCommandList(nvrhi::ICommandList* commandList);
		void ExecuteCommandListLocked(nvrhi::ICommandList* commandList);

		void Lock() { m_ExecutionMutex.lock(); }
		void Unlock() { m_ExecutionMutex.unlock(); }

		void ExecuteCommand(auto cmd)
		{
			m_CommandList->open();
			cmd(m_CommandList);
			m_CommandList->close();

			ExecuteCommandListLocked(m_CommandList);
		}

	public:
		DeviceManager();
		virtual ~DeviceManager() = default;

	protected:
		virtual bool CreateDeviceInternal() = 0;

	protected:
		DeviceSpecification m_Sepcification;
		nvrhi::DeviceHandle m_NvrhiDevice;
		nvrhi::CommandListHandle m_CommandList;

		std::mutex m_ExecutionMutex;
	};

}
