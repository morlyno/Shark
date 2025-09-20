#pragma once

#include "Shark/Core/Base.h"

#include <nvrhi/nvrhi.h>

namespace Shark {
	class CommandList;
}

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

		virtual void LockCommandList(nvrhi::ICommandList* commandList) = 0;
		virtual void UnlockCommandList(nvrhi::ICommandList* commandList) = 0;
		virtual void ExecuteCommandList(nvrhi::ICommandList* commandList) = 0;
		virtual void ExecuteCommandListLocked(nvrhi::ICommandList* commandList) = 0;

		void ExecuteCommandList(CommandList* commandList);
		void ExecuteCommandListLocked(CommandList* commandList);

		virtual void Lock() = 0;
		virtual void Unlock() = 0;

		virtual CommandList* GetCommandList(nvrhi::CommandQueue queue) = 0;

	protected:
		virtual bool CreateDeviceInternal() = 0;

	protected:
		DeviceSpecification m_Sepcification;
	};

	class CommandList
	{
	public:
		CommandList(DeviceManager* deviceManager, nvrhi::CommandListHandle commandList);
		~CommandList();

		CommandList(const CommandList&) = delete;
		CommandList(CommandList&&) = delete;
		CommandList& operator=(const CommandList&) = delete;
		CommandList& operator=(CommandList&&) = delete;

	public:
		nvrhi::CommandListHandle GetHandle() const { return m_CommandList; }

		void open();
		void close();

		void writeTexture(nvrhi::ITexture* dest, uint32_t arraySlice, uint32_t mipLevel, const void* data, size_t rowPitch, size_t depthPitch = 0);
		void writeBuffer(nvrhi::IBuffer* b, const void* data, size_t dataSize, uint64_t destOffsetBytes = 0);

	private:
		DeviceManager* m_DeviceManager;
		nvrhi::CommandListHandle m_CommandList;

		friend class DeviceManager;
	};

}
