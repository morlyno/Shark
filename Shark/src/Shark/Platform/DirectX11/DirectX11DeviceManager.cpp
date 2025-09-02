#include "skpch.h"
#include "DirectX11DeviceManager.h"

namespace Shark {

	class MessageCallback : public nvrhi::IMessageCallback
	{
	public:
		virtual void message(nvrhi::MessageSeverity severity, const char* messageText) override
		{
			switch (severity)
			{
				case nvrhi::MessageSeverity::Info: SK_CORE_INFO_TAG("Renderer", messageText); break;
				case nvrhi::MessageSeverity::Warning: SK_CORE_WARN_TAG("Renderer", messageText); break;
				case nvrhi::MessageSeverity::Error: SK_CORE_ERROR_TAG("Renderer", messageText); break;
				case nvrhi::MessageSeverity::Fatal: SK_CORE_CRITICAL_TAG("Renderer", messageText); break;
			}
		}

		static MessageCallback& GetInstance()
		{
			static MessageCallback instance;
			return instance;
		}
	};

	bool DirectX11DeviceManager::CreateDeviceInternal()
	{
		HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_Factory));
		if (FAILED(hr))
		{
			SK_CORE_ERROR_TAG("Renderer", "Failed to create DXGI Factory!");
			return false;
		}

		int adapterIndex = m_Sepcification.AdapterIndex;
		if (adapterIndex < 0)
			adapterIndex = 0;

		if (FAILED(m_Factory->EnumAdapters(adapterIndex, &m_Adapter)))
		{
			if (adapterIndex == 0)
				SK_CORE_ERROR_TAG("Renderer", "Cannot find any DXGI adapters!");
			else
				SK_CORE_ERROR_TAG("Renderer", "The specified DXGI adapter %d does not exist", adapterIndex);
			return false;
		}

		UINT createFlags = 0;
		if (m_Sepcification.EnableDebugRuntime)
			createFlags |= D3D11_CREATE_DEVICE_DEBUG;

		auto featureLevel = D3D_FEATURE_LEVEL_11_1;

		hr = D3D11CreateDevice(m_Adapter,
							   D3D_DRIVER_TYPE_UNKNOWN,
							   nullptr,
							   createFlags,
							   &featureLevel,
							   1,
							   D3D11_SDK_VERSION,
							   &m_Device,
							   nullptr,
							   &m_ImmediateContext);

		if (FAILED(hr))
			return false;

		nvrhi::d3d11::DeviceDesc deviceDesc;
		deviceDesc.messageCallback = &MessageCallback::GetInstance();
		deviceDesc.context = m_ImmediateContext;
		m_NvrhiDevice = nvrhi::d3d11::createDevice(deviceDesc);

		if (m_Sepcification.EnableNvrhiValidationLayer)
		{
			m_NvrhiDevice = nvrhi::validation::createValidationLayer(m_NvrhiDevice);
		}
	}

	HRESULT DirectX11DeviceManager::CreateSwapChain(DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** outSwapChain)
	{
		return m_Factory->CreateSwapChain(m_Device, desc, outSwapChain);
	}

}
