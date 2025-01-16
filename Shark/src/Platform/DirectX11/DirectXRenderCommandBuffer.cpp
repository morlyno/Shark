#include "skpch.h"
#include "DirectXRenderCommandBuffer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXAPI.h"
#include "Platform/DirectX11/DirectXContext.h"
#include "Platform/DirectX11/DirectXRenderer.h"

#include "Platform/Windows/WindowsUtils.h"

namespace Shark {

	DirectXRenderCommandBuffer::DirectXRenderCommandBuffer(const std::string& name)
		: m_DebugName(name)
	{
		CreateDeferredContext();
		CreateQueries();
	}

	DirectXRenderCommandBuffer::~DirectXRenderCommandBuffer()
	{
		Release();
	}

	void DirectXRenderCommandBuffer::Release()
	{
		if (!m_Context)
			return;

		Renderer::SubmitResourceFree([context = m_Context, annotation = m_Annotation, commandList = m_CommandList, pipelineQueries = m_PipelineStatsQueries, timestampQueryPools = m_TimestampQueryPools]()
		{
			if (commandList)
				commandList->Release();
			if (annotation)
				annotation->Release();
			if (context)
				context->Release();

			for (auto query : pipelineQueries)
				DirectXAPI::ReleaseObject(query);

			for (auto& pool : timestampQueryPools)
			{
				for (auto& query : pool)
				{
					DirectXAPI::ReleaseObject(query.first);
					DirectXAPI::ReleaseObject(query.second);
				}
			}
		});

		m_CommandList = nullptr;
		m_Context = nullptr;
		m_PipelineStatsQueries.fill(nullptr);
		m_TimestampQueryPools.fill({});
	}

	void DirectXRenderCommandBuffer::ReleaseCommandList()
	{
		DirectXAPI::ReleaseObject(m_CommandList);
		m_CommandList = nullptr;
	}

	void DirectXRenderCommandBuffer::Begin()
	{
		m_Active = true;

		const uint32_t poolIndex = Renderer::GetCurrentFrameIndex() % m_TimestampQueryPools.size();
		m_TimestampQueryCount[poolIndex] = 0;
		m_NextAvailableQueryIndex = 0;


		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			uint32_t index = Renderer::RT_GetCurrentFrameIndex() % instance->m_PipelineStatsQueries.size();
			instance->m_Context->Begin(instance->m_PipelineStatsQueries[index]);
		});
	}

	void DirectXRenderCommandBuffer::End()
	{
		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance]()
		{
			uint32_t index = Renderer::RT_GetCurrentFrameIndex() % instance->m_PipelineStatsQueries.size();
			instance->m_Context->End(instance->m_PipelineStatsQueries[index]);

			DirectXAPI::ReleaseObject(instance->m_CommandList);
			DX11_VERIFY(instance->m_Context->FinishCommandList(FALSE, &instance->m_CommandList));
		});
		m_Active = false;
	}

	void DirectXRenderCommandBuffer::Execute(bool releaseCommandList)
	{
		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance, releaseCommandList]()
		{
			SK_PROFILE_SCOPED("DirectXRenderCommandBuffer::Execute");
			SK_PERF_SCOPED("Execute RenderCommandBuffer");
			auto device = DirectXContext::GetCurrentDevice();

			device->Lock();
			auto queue = device->GetQueue();
			queue->ExecuteCommandList(instance->m_CommandList, FALSE);

			if (releaseCommandList)
			{
				DirectXAPI::ReleaseObject(instance->m_CommandList);
				instance->m_CommandList = nullptr;
			}

			const uint32_t getdataIndex = (Renderer::RT_GetCurrentFrameIndex() + 1) % (uint32_t)instance->m_PipelineStatsQueries.size();

			D3D11_QUERY_DATA_PIPELINE_STATISTICS stats;
			HRESULT hr = queue->GetData(instance->m_PipelineStatsQueries[getdataIndex], &stats, sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS), 0);
			if (hr == S_OK)
			{
				instance->m_PipelineStatistics.InputAssemblerVertices = stats.IAVertices;
				instance->m_PipelineStatistics.InputAssemblerPrimitives = stats.IAPrimitives;
				instance->m_PipelineStatistics.VertexShaderInvocations = stats.VSInvocations;
				instance->m_PipelineStatistics.PixelShaderInvocations = stats.PSInvocations;
				instance->m_PipelineStatistics.ComputeShaderInvocations = stats.CSInvocations;
				instance->m_PipelineStatistics.RasterizerInvocations = stats.CInvocations;
				instance->m_PipelineStatistics.RasterizerPrimitives = stats.CPrimitives;
			}

			const uint64_t gpuFrequency = DirectXContext::GetCurrentDevice()->GetLimits().TimestampPeriod;

			const uint32_t count = instance->m_TimestampQueryCount[getdataIndex];
			instance->m_TimestampQueryResults[getdataIndex].resize(count);
			for (uint32_t index = 0; index < count; index++)
			{
				auto& [startQuery, endQuery] = instance->m_TimestampQueryPools[getdataIndex][index];
				HRESULT hrStart, hrEnd;
				uint64_t startTime, endTime;
				hrStart = queue->GetData(startQuery, &startTime, sizeof(uint64_t), 0);
				hrEnd = queue->GetData(endQuery, &endTime, sizeof(uint64_t), 0);

				uint64_t sTime = 0;
				if (hrStart == S_OK && hrEnd == S_OK)
				{
					sTime = endTime - startTime;
				}

				instance->m_TimestampQueryResults[getdataIndex][index] = (float)sTime / gpuFrequency;
			}

			device->Unlock();
		});
	}

	TimeStep DirectXRenderCommandBuffer::GetTime(uint32_t queryID) const
	{
		SK_CORE_VERIFY(!m_Active);
		const uint32_t getdataIndex = (Renderer::GetCurrentFrameIndex() + 1) % (uint32_t)m_PipelineStatsQueries.size();
		auto& pool = m_TimestampQueryResults[getdataIndex];
		if (queryID < pool.size())
			return pool[queryID];
		return {};
	}

	uint32_t DirectXRenderCommandBuffer::BeginTimestampQuery()
	{
		SK_CORE_VERIFY(m_Active);
		const uint32_t poolIndex = Renderer::GetCurrentFrameIndex() % m_TimestampQueryPools.size();
		const uint32_t queryIndex = m_NextAvailableQueryIndex++;
		if (m_TimestampQueryPools[poolIndex].size() == queryIndex)
		{
			auto device = DirectXContext::GetCurrentDevice();
			auto dxDevice = device->GetDirectXDevice();

			ID3D11Query* startQuery = nullptr;
			ID3D11Query* endQuery = nullptr;
			DirectXAPI::CreateQuery(dxDevice, D3D11_QUERY_TIMESTAMP, startQuery);
			DirectXAPI::CreateQuery(dxDevice, D3D11_QUERY_TIMESTAMP, endQuery);
			m_TimestampQueryPools[poolIndex].emplace_back(startQuery, endQuery);
		}

		m_TimestampQueryCount[poolIndex]++;
		auto [beginQuery, endQuery] = m_TimestampQueryPools[poolIndex][queryIndex];
		SK_CORE_VERIFY(beginQuery);

		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance, beginQuery]() { instance->m_Context->End(beginQuery); });
		return queryIndex;
	}

	void DirectXRenderCommandBuffer::EndTimestampQuery(uint32_t queryID)
	{
		SK_CORE_VERIFY(m_Active);
		const uint32_t index = Renderer::GetCurrentFrameIndex() % m_TimestampQueryPools.size();
		SK_CORE_VERIFY(queryID < m_TimestampQueryPools[index].size(), "Invalid timestamp query ID {}", queryID);
		auto [beginQuery, endQuery] = m_TimestampQueryPools[index][queryID];

		Ref<DirectXRenderCommandBuffer> instance = this;
		Renderer::Submit([instance, endQuery]() { instance->m_Context->End(endQuery); });
	}

	void DirectXRenderCommandBuffer::CreateQueries()
	{
		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		D3D11_QUERY_DESC desc = {};
		desc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
		for (uint32_t i = 0; i < m_PipelineStatsQueries.size(); i++)
			dxDevice->CreateQuery(&desc, &m_PipelineStatsQueries[i]);
	}

	void DirectXRenderCommandBuffer::CreateDeferredContext()
	{
		auto device = DirectXContext::GetCurrentDevice();
		auto dxDevice = device->GetDirectXDevice();

		DirectXAPI::CreateDeferredContext(device, m_Context);
		DirectXAPI::SetDebugName(m_Context, m_DebugName);

		m_Context->QueryInterface(IID_PPV_ARGS(&m_Annotation));
	}

	DirectXRenderCommandBuffer::TimeQuery DirectXRenderCommandBuffer::GetNextAvailableTimeQuery(uint32_t& outID)
	{
		const uint32_t index = Renderer::GetCurrentFrameIndex() % m_TimestampQueryPools.size();
		if (m_TimestampQueryPools[index].size() == m_NextAvailableQueryIndex)
		{
			auto device = DirectXContext::GetCurrentDevice();
			auto dxDevice = device->GetDirectXDevice();

			ID3D11Query* startQuery = nullptr;
			ID3D11Query* endQuery = nullptr;
			DirectXAPI::CreateQuery(dxDevice, D3D11_QUERY_TIMESTAMP, startQuery);
			DirectXAPI::CreateQuery(dxDevice, D3D11_QUERY_TIMESTAMP, endQuery);
			m_TimestampQueryPools[index].emplace_back(startQuery, endQuery);
		}

		outID = m_NextAvailableQueryIndex;
		return m_TimestampQueryPools[index][m_NextAvailableQueryIndex++];
	}

	DirectXRenderCommandBuffer::TimeQuery DirectXRenderCommandBuffer::GetTimeQuery(uint32_t id)
	{
		const uint32_t index = Renderer::GetCurrentFrameIndex() % m_TimestampQueryPools.size();
		if (id >= m_TimestampQueryPools[index].size())
			return { nullptr, nullptr };

		return m_TimestampQueryPools[index][id];
	}

}
