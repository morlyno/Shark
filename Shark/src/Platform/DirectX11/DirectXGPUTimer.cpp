#include "skpch.h"
#include "DirectXGPUTimer.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXRenderer.h"

namespace Shark {

	DirectXGPUTimer::DirectXGPUTimer(const std::string& name)
		: m_Name(name)
	{
		Ref<DirectXGPUTimer> instance = this;
		Renderer::Submit([instance]()
		{
			auto device = DirectXRenderer::GetDevice();
			const auto startQueryName = fmt::format("{} (Start)", instance->m_Name);
			const auto endQueryName = fmt::format("{} (End)", instance->m_Name);

			D3D11_QUERY_DESC desc;
			desc.Query = D3D11_QUERY_TIMESTAMP;
			desc.MiscFlags = 0;
			for (uint32_t i = 0; i < NumQueries; i++)
			{
				SK_DX11_CALL(device->CreateQuery(&desc, &instance->m_StartQuery[i]));
				SK_DX11_CALL(device->CreateQuery(&desc, &instance->m_EndQuery[i]));

				D3D_SET_OBJECT_NAME_A(instance->m_StartQuery[i], startQueryName.c_str());
				D3D_SET_OBJECT_NAME_A(instance->m_EndQuery[i], endQueryName.c_str());
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

	DirectXGPUPipelineQuery::DirectXGPUPipelineQuery(const std::string& name)
	{
		auto renderer = DirectXRenderer::Get();
		ID3D11Device* device = renderer->GetDevice();

		D3D11_QUERY_DESC desc = {};
		desc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
		for (uint32_t i = 0; i < m_Queries.size(); i++)
		{
			DX11_VERIFY(device->CreateQuery(&desc, &m_Queries[i]));
		}

		m_Index = 0;
		m_DataIndex = (m_Index + 1) % (uint32_t)m_Queries.size();
	}

	DirectXGPUPipelineQuery::~DirectXGPUPipelineQuery()
	{
		if (!m_Queries[0])
			return;

		Renderer::SubmitResourceFree([queries = m_Queries]()
		{
			for (auto& query : queries)
				DirectXAPI::ReleaseObject(query);
		});

		m_Queries.fill(nullptr);
	}

	void DirectXGPUPipelineQuery::Begin(ID3D11DeviceContext* context)
	{
		context->Begin(m_Queries[m_Index]);
	}

	void DirectXGPUPipelineQuery::End(ID3D11DeviceContext* context)
	{
		context->End(m_Queries[m_Index]);

		D3D11_QUERY_DATA_PIPELINE_STATISTICS stats;
		HRESULT hResult = context->GetData(m_Queries[m_DataIndex], &stats, sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS), 0);

		if (hResult == S_OK)
		{
			m_Statisitics.InputAssemblerVertices = stats.IAVertices;
			m_Statisitics.InputAssemblerPrimitives = stats.IAPrimitives;
			m_Statisitics.VertexShaderInvocations = stats.VSInvocations;
			m_Statisitics.PixelShaderInvocations = stats.PSInvocations;
			m_Statisitics.ComputeShaderInvocations = stats.CSInvocations;
			m_Statisitics.RasterizerInvocations = stats.CInvocations;
			m_Statisitics.RasterizerPrimitives = stats.CPrimitives;
		}

		++m_Index %= m_Queries.size();
		++m_DataIndex %= m_Queries.size();
	}

}
