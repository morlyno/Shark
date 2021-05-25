#include "skpch.h"
#include "DirectXRendererAPI.h"
#include "Shark/Core/Application.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR(SK_STRINGIFY(call) "0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	DirectXRendererAPI* DirectXRendererAPI::s_Instance = nullptr;

	namespace Utils {

		static void LogAdapter(IDXGIAdapter* adapter)
		{
			DXGI_ADAPTER_DESC ad;
			SK_CHECK(adapter->GetDesc(&ad));
			char gpudesc[128];
			wcstombs_s(nullptr, gpudesc, ad.Description, 128);
			SK_CORE_INFO("GPU: {0}", gpudesc);
		}

	}

	void DirectXRendererAPI::Init()
	{
		SK_CORE_ASSERT(s_Instance == nullptr);
		s_Instance = this;

		SK_CHECK(CreateDXGIFactory(IID_PPV_ARGS(&m_Factory)));

		IDXGIAdapter* adapter = nullptr;
		SK_CHECK(m_Factory->EnumAdapters(0, &adapter));
		Utils::LogAdapter(adapter);

		UINT createdeviceFalgs = 0u;
#ifdef SK_DEBUG
		createdeviceFalgs |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		SK_CHECK(D3D11CreateDevice(
			adapter,
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			createdeviceFalgs,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&m_Device,
			nullptr,
			&m_Context
		));

		adapter->Release();

		auto& window = Application::Get().GetWindow();

		SwapChainSpecifications specs;
		specs.Widht = window.GetWidth();
		specs.Height = window.GetHeight();
		specs.WindowHandle = window.GetHandle();
		m_SwapChain = Ref<DirectXSwapChain>::Create(specs);

		m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void DirectXRendererAPI::ShutDown()
	{
		m_SwapChain.Release();
		if (m_Device) { m_Device->Release(); m_Device = nullptr; }
		if (m_Context) { m_Context->Release(); m_Context = nullptr; }
		if (m_Factory) { m_Factory->Release(); m_Factory = nullptr; }

		s_Instance = nullptr;
	}

	void DirectXRendererAPI::DrawIndexed(uint32_t count, uint32_t indexoffset, uint32_t vertexoffset)
	{
		m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_Context->DrawIndexed(count, indexoffset, vertexoffset);
	}

	void DirectXRendererAPI::Flush()
	{
		m_Context->Flush();
	}

}