#include "skpch.h"
#include "DirectXRendererAPI.h"
#include "Shark/Core/Application.h"

#include "Shark/Debug/Instrumentor.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR(SK_STRINGIFY(call) "0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	static D3D11_PRIMITIVE_TOPOLOGY SharkPrimitveTopologyToD3D11(PrimitveTopology topology)
	{
		switch (topology)
		{
			case PrimitveTopology::Triangle:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case PrimitveTopology::Line:      return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			case PrimitveTopology::Dot:       return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		}

		SK_CORE_ASSERT(false, "Unkonw Topology");
		return (D3D11_PRIMITIVE_TOPOLOGY)0;
	}

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
		SK_PROFILE_FUNCTION();

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
			&m_ImmediateContext
		));
		m_ActiveContext = m_ImmediateContext;

		adapter->Release();

		auto& window = Application::Get().GetWindow();

		SwapChainSpecifications specs;
		specs.Widht = window.GetWidth();
		specs.Height = window.GetHeight();
		specs.WindowHandle = window.GetHandle();
		m_SwapChain = Ref<DirectXSwapChain>::Create(specs);
	}

	void DirectXRendererAPI::ShutDown()
	{
		m_SwapChain.Release();
		if (m_Device) { m_Device->Release(); m_Device = nullptr; }
		if (m_ImmediateContext) { m_ImmediateContext->Release(); m_ImmediateContext = nullptr; }
		if (m_Factory) { m_Factory->Release(); m_Factory = nullptr; }

		s_Instance = nullptr;
	}

	void DirectXRendererAPI::SetBlendForImgui(bool blend)
	{
		if (blend)
		{
			SK_CORE_ASSERT(m_ImGuiBlendState);
			const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
			m_ImmediateContext->OMSetBlendState(m_ImGuiBlendState, blend_factor, 0xFFFFFFFF);
		}
		else
		{
			m_ImmediateContext->OMGetBlendState(&m_ImGuiBlendState, nullptr, nullptr);
			ID3D11BlendState* nullBlend = nullptr;
			const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
			m_ImmediateContext->OMSetBlendState(nullBlend, nullptr, 0xFFFFFFFF);
		}
	}

	void DirectXRendererAPI::Draw(uint32_t vertexCount, PrimitveTopology topology)
	{
		m_ActiveContext->IASetPrimitiveTopology(SharkPrimitveTopologyToD3D11(topology));
		m_ActiveContext->Draw(vertexCount, 0);
	}

	void DirectXRendererAPI::DrawIndexed(uint32_t indexCount, PrimitveTopology topology)
	{
		m_ActiveContext->IASetPrimitiveTopology(SharkPrimitveTopologyToD3D11(topology));
		m_ActiveContext->DrawIndexed(indexCount, 0, 0);
	}

}