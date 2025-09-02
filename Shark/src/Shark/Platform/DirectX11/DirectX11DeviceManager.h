#pragma once

#include "Shark/Render/DeviceManager.h"

#include "nvrhi/nvrhi.h"
#include "nvrhi/validation.h"
#include "nvrhi/d3d11.h"

namespace Shark {

	class DirectX11DeviceManager : public DeviceManager
	{
	public:
		virtual bool CreateDeviceInternal() override;

		virtual nvrhi::IDevice* GetDevice() const override { return m_NvrhiDevice; }
		virtual nvrhi::GraphicsAPI GetGraphicsAPI() const override { return nvrhi::GraphicsAPI::D3D11; }

		IDXGIFactory1* GetFactory() { return m_Factory; }

		HRESULT CreateSwapChain(DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** outSwapChain);
	private:
		nvrhi::DeviceHandle m_NvrhiDevice;

		nvrhi::RefCountPtr<ID3D11Device> m_Device;
		nvrhi::RefCountPtr<IDXGIFactory1> m_Factory;
		nvrhi::RefCountPtr<ID3D11DeviceContext> m_ImmediateContext;
		nvrhi::RefCountPtr<IDXGIAdapter> m_Adapter;
	};

}