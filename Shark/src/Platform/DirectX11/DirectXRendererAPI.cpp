#include "skpch.h"
#include "DirectXRendererAPI.h"
#include <stdlib.h>

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR(SK_STRINGIFY(call) "0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	void DirectXRendererAPI::Init()
	{
		SK_CORE_ASSERT(m_Device == nullptr, "RendererAPI already initialized");
		SK_CORE_INFO("Init RendererAPI");

		SK_CHECK(CreateDXGIFactory(IID_PPV_ARGS(&m_Factory)));

		IDXGIAdapter* gpu = nullptr;
		if (HRESULT hr = m_Factory->EnumAdapters(0u, &gpu); FAILED(hr))
		{
			SK_CORE_ASSERT(hr != DXGI_ERROR_INVALID_CALL);
			if (hr == DXGI_ERROR_NOT_FOUND)
			{
				SK_CORE_CRITICAL("!!! No Adapter could be found !!!");
				throw std::exception("Failed to find a GPU Adapter");
			}
		}

		{
			DXGI_ADAPTER_DESC ad;
			SK_CHECK(gpu->GetDesc(&ad));
			char gpudesc[128];
			wcstombs_s(nullptr, gpudesc, ad.Description, 128);
			SK_CORE_INFO("GPU: {0}", gpudesc);
		}

		UINT createdeviceFalgs = 0u;
#if SK_DEBUG
		createdeviceFalgs |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		SK_CHECK(D3D11CreateDevice(
			gpu,
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

		if (gpu) { gpu->Release(); gpu = nullptr; }
		
		// TODO: Make chanchable in the future
		m_Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void DirectXRendererAPI::ShutDown()
	{
		if (m_Device) { m_Device->Release(); m_Device = nullptr; }
		if (m_Context) { m_Context->Release(); m_Context = nullptr; }
		if (m_Factory) { m_Factory->Release(); m_Factory = nullptr; }
	}

	void DirectXRendererAPI::DrawIndexed(uint32_t count)
	{
		m_Context->DrawIndexed(count, 0u, 0u);
	}

	void DirectXRendererAPI::Flush()
	{
		m_Context->Flush();
	}

}