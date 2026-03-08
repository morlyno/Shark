#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Enum.h"
#include "Shark/Render/SwapChain.h"

#include <nvrhi/nvrhi.h>
#include <shared_mutex>

namespace Shark {

	struct DeviceSpecification
	{
		int AdapterIndex = -1;
		uint32_t SwapchainBufferCount = 3;
		uint32_t MaxFramesInFlight = 2;

		bool AllowModeSwitch = false;
		bool EnableDebugRuntime = false;
		bool EnableNvrhiValidationLayer = false;

		bool Headless = false;
		bool EnableComputeQueue = true;
		bool EnableCopyQueue = true;

		bool SrgbSurface = false;
		nvrhi::Format SurfaceFormat = nvrhi::Format::UNKNOWN;
		WindowHandle Window = nullptr;
	};

	class DeviceManager
	{
	public:
		static Scope<DeviceManager> Create(nvrhi::GraphicsAPI api);
		bool CreateDevice(const DeviceSpecification& specification);

		virtual void RunGarbageCollection();

		virtual Ref<SwapChain> CreateSwapchain(const SwapChainSpecification& specification) = 0;

		bool Initialized() const { return m_NvrhiDevice != nullptr; }
		virtual nvrhi::IDevice* GetDevice() const = 0;
		virtual nvrhi::GraphicsAPI GetGraphicsAPI() const = 0;
		const DeviceSpecification& GetSpecification() const { return m_Specification; }

		void ExecuteCommandList(nvrhi::ICommandList* commandList);
		void ExecuteCommandListLocked(nvrhi::ICommandList* commandList);

		void LockQueue() { m_ExecutionMutex.lock(); }
		void UnlockQueue() { m_ExecutionMutex.unlock(); }

		auto ExecuteCommand(auto&& cmd) { ExecuteCommand(nvrhi::CommandQueue::Graphics, cmd); }
		auto ExecuteCommand(nvrhi::CommandQueue queue, auto&& cmd)
		{
			auto commandList = GetTemporaryCommandList(queue);
			commandList->open();
			cmd(commandList);
			commandList->close();
			ExecuteCommandListLocked(commandList);
		}

		nvrhi::CommandListHandle GetTemporaryCommandList(nvrhi::CommandQueue queue = nvrhi::CommandQueue::Graphics);

	public:
		DeviceManager();
		virtual ~DeviceManager();

	protected:
		virtual void DestroyInternal() {}
		virtual bool CreateInstanceInternal() = 0;
		virtual bool CreateDeviceInternal() = 0;
		virtual void RunGarbageCollectionInternal() {}

		nvrhi::CommandListHandle GetOrCreateThreadLocalCommandList(nvrhi::CommandQueue queue);

	protected:
		DeviceSpecification m_Specification;
		nvrhi::DeviceHandle m_NvrhiDevice;

		std::shared_mutex m_CommandListMutex;
		std::unordered_map<std::thread::id, Enum::Array<nvrhi::CommandQueue, nvrhi::CommandListHandle>> m_CommandLists;

		std::mutex m_ExecutionMutex;
	};

	class MessageCallback : public nvrhi::IMessageCallback
	{
	public:
		virtual void message(nvrhi::MessageSeverity severity, const char* messageText) override
		{
			switch (severity)
			{
				case nvrhi::MessageSeverity::Info: SK_CORE_INFO_TAG("nvrhi", messageText); break;
				case nvrhi::MessageSeverity::Warning: SK_CORE_WARN_TAG("nvrhi", messageText); break;
				case nvrhi::MessageSeverity::Error: SK_CORE_ERROR_TAG("nvrhi", messageText); break;
				case nvrhi::MessageSeverity::Fatal: SK_CORE_CRITICAL_TAG("nvrhi", messageText); break;
			}

			if (severity >= nvrhi::MessageSeverity::Error)
			{
				SK_DEBUG_BREAK_CONDITIONAL(s_BREAK_ON_MESSAGE);
			}

		}

		static MessageCallback& GetInstance()
		{
			static MessageCallback instance;
			return instance;
		}
	};

}
