#pragma once

#include "Shark/Render/DeviceManager.h"

#include "nvrhi/nvrhi.h"
#include "nvrhi/validation.h"
#include "nvrhi/d3d11.h"

#include <semaphore>

namespace Shark {

	class DirectX11DeviceManager : public DeviceManager
	{
	public:
		virtual bool CreateDeviceInternal() override;

		virtual nvrhi::IDevice* GetDevice() const override { return m_NvrhiDevice; }
		virtual nvrhi::GraphicsAPI GetGraphicsAPI() const override { return nvrhi::GraphicsAPI::D3D11; }

		virtual void LockCommandList(nvrhi::ICommandList* commandList) override;
		virtual void UnlockCommandList(nvrhi::ICommandList* commandList) override;
		virtual void ExecuteCommandList(nvrhi::ICommandList* commandList) override;
		virtual void ExecuteCommandListLocked(nvrhi::ICommandList* commandList) override;

		virtual void Lock() override { m_ExecutionMutex.lock(); }
		virtual void Unlock() override { m_ExecutionMutex.unlock(); }

		virtual CommandList* GetCommandList(nvrhi::CommandQueue queue) override { return m_CommandList.Raw(); }

		IDXGIFactory1* GetFactory() { return m_Factory; }

		HRESULT CreateSwapChain(DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** outSwapChain);
	private:
		nvrhi::DeviceHandle m_NvrhiDevice;

		nvrhi::RefCountPtr<ID3D11Device> m_Device;
		nvrhi::RefCountPtr<IDXGIFactory1> m_Factory;
		nvrhi::RefCountPtr<ID3D11DeviceContext> m_ImmediateContext;
		nvrhi::RefCountPtr<IDXGIAdapter> m_Adapter;

		Scope<CommandList> m_CommandList;
		std::binary_semaphore m_CommandListSemaphore{ 1 };
		std::mutex m_ExecutionMutex;
	};

}
