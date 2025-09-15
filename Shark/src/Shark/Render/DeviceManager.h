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
		static DeviceManager* Create(nvrhi::GraphicsAPI api);
		bool CreateDevice(const DeviceSpecification& specification);

		virtual nvrhi::IDevice* GetDevice() const = 0;
		virtual nvrhi::GraphicsAPI GetGraphicsAPI() const = 0;
		const DeviceSpecification& GetSpecification() const { return m_Sepcification; }

		virtual void OnOpenCommandList(nvrhi::ICommandList* commandList) = 0;
		virtual void OnCloseCommandList(nvrhi::ICommandList* commandList) = 0;
		virtual void ExecuteCommandList(nvrhi::ICommandList* commandList) = 0;

		virtual nvrhi::ICommandList* GetCommandList(nvrhi::CommandQueue queue) = 0;

	protected:
		virtual bool CreateDeviceInternal() = 0;

	protected:
		DeviceSpecification m_Sepcification;
	};

}
