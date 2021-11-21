#include "skpch.h"
#include "DirectXGPUTimer.h"

#include "Shark/Utility/Utility.h"

#include "Platform/DirectX11/DirectXRenderer.h"

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR(SK_STRINGIFY(call) " 0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	DirectXGPUTimer::DirectXGPUTimer(const std::string& name)
		: m_Name(name)
	{
		auto dev = DirectXRenderer::GetDevice();

		D3D11_QUERY_DESC desc;
		desc.Query = D3D11_QUERY_TIMESTAMP;
		desc.MiscFlags = 0;
		for (uint32_t i = 0; i < NumQueries; i++)
		{
			SK_CHECK(dev->CreateQuery(&desc, &m_StartQuery[i]));
			SK_CHECK(dev->CreateQuery(&desc, &m_EndQuery[i]));
		}
	}

	DirectXGPUTimer::~DirectXGPUTimer()
	{
		for (uint32_t i = 0; i < NumQueries; i++)
		{
			if (m_StartQuery[i])
				m_StartQuery[i]->Release();
			if (m_EndQuery[i])
				m_EndQuery[i]->Release();
		}
	}

	void DirectXGPUTimer::StartQuery(ID3D11DeviceContext* targetContext)
	{
		targetContext->End(m_StartQuery[m_Index]);
	}

	void DirectXGPUTimer::EndQuery(ID3D11DeviceContext* targetContext)
	{
		targetContext->End(m_EndQuery[m_Index]);

		UpdateTime();
		NextIndex();
	}

	void DirectXGPUTimer::NextIndex()
	{
		++m_Index %= NumQueries;
		m_DataIndex = (m_Index + 1) % NumQueries;
	}

	void DirectXGPUTimer::UpdateTime()
	{
		ID3D11DeviceContext* ctx = DirectXRenderer::GetContext();

		Ref<DirectXRenderer> dxRenderer = DirectXRenderer::Get();
		if (!dxRenderer->HasValidFrequncy())
			return;

		uint64_t startTime;
		HRESULT hr = ctx->GetData(m_StartQuery[m_DataIndex], &startTime, sizeof(uint64_t), 0);
		if (hr != S_OK)
			return;

		uint64_t endTime;
		hr = ctx->GetData(m_EndQuery[m_DataIndex], &endTime, sizeof(uint64_t), 0);
		if (hr != S_OK)
			return;

		uint64_t durration = endTime - startTime;
		m_LastTickCount = durration;
		m_LastFrequency = dxRenderer->GetGPUFrequncy();
	}

}
