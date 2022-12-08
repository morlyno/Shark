#include "skpch.h"
#include "DirectXGPUTimer.h"

#include "Platform/DirectX11/DirectXRenderer.h"
#include "Shark/Render/Renderer.h"

namespace Shark {

	DirectXGPUTimer::DirectXGPUTimer(const std::string& name)
		: m_Name(name)
	{
		Ref<DirectXGPUTimer> instance = this;
		Renderer::Submit([instance]()
		{
			auto device = DirectXRenderer::GetDevice();

			D3D11_QUERY_DESC desc;
			desc.Query = D3D11_QUERY_TIMESTAMP;
			desc.MiscFlags = 0;
			for (uint32_t i = 0; i < NumQueries; i++)
			{
				SK_DX11_CALL(device->CreateQuery(&desc, &instance->m_StartQuery[i]));
				SK_DX11_CALL(device->CreateQuery(&desc, &instance->m_EndQuery[i]));
			}
		});
	}

	DirectXGPUTimer::~DirectXGPUTimer()
	{
		ID3D11Query* startQueries[NumQueries];
		ID3D11Query* endQueries[NumQueries];
		memcpy(startQueries, m_StartQuery, sizeof(startQueries));
		memcpy(endQueries, m_EndQuery, sizeof(endQueries));

		Renderer::SubmitResourceFree([startQueries, endQueries, count = NumQueries]()
		{
			for (uint32_t i = 0; i < count; i++)
			{
				if (startQueries[i])
					startQueries[i]->Release();
				if (endQueries[i])
					endQueries[i]->Release();
			}
		});

		for (auto& q : m_StartQuery)
			q = nullptr;

		for (auto& q : m_EndQuery)
			q = nullptr;
	}

	void DirectXGPUTimer::RT_StartQuery(ID3D11DeviceContext* targetContext)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		targetContext->End(m_StartQuery[m_Index]);
	}

	void DirectXGPUTimer::RT_EndQuery(ID3D11DeviceContext* targetContext)
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

		targetContext->End(m_EndQuery[m_Index]);

		RT_UpdateTime();
		NextIndex();
	}

	void DirectXGPUTimer::NextIndex()
	{
		++m_Index %= NumQueries;
		m_DataIndex = (m_Index + 1) % NumQueries;
	}

	void DirectXGPUTimer::RT_UpdateTime()
	{
		SK_CORE_VERIFY(Renderer::IsOnRenderThread());

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
