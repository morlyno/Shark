#include "skpch.h"
#include "RenderCommandBuffer.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	RenderCommandBuffer::RenderCommandBuffer(const std::string& name, uint32_t queryCountHint)
		: m_Name(name)
	{
		auto deviceManager = Application::Get().GetDeviceManager();
		auto device = deviceManager->GetDevice();

		// NOTE(moro): nvrhi doesn't support deferred command lists for d3d11
		const bool deferredSupported = device->queryFeatureSupport(nvrhi::Feature::DeferredCommandLists);

		auto desc = nvrhi::CommandListParameters()
			.setEnableImmediateExecution(!deferredSupported)
			.setQueueType(nvrhi::CommandQueue::Graphics);

		m_CommandList = device->createCommandList(desc);

		uint32_t poolCount = deviceManager->GetSpecification().SwapchainBufferCount;
		m_TimerQueryPools.resize(poolCount);
		m_GPUExecutionTimes.resize(poolCount);

		for (uint32_t poolIndex = 0; poolIndex < poolCount; poolIndex++)
		{
			m_TimerQueryPools.reserve(queryCountHint + 1);
			m_GPUExecutionTimes.reserve(queryCountHint + 1);
		}

		m_TimerQuery = RegisterTimerQuery();
	}

	RenderCommandBuffer::~RenderCommandBuffer()
	{
	}

	void RenderCommandBuffer::Begin()
	{
		Ref instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_Begin();
		});
	}

	void RenderCommandBuffer::End()
	{
		Ref instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_End();
		});
	}

	void RenderCommandBuffer::Execute()
	{
		Ref instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_Execute();
		});
	}

	void RenderCommandBuffer::RT_Begin()
	{
		SK_PROFILE_FUNCTION();

		Application::Get().GetDeviceManager()->LockCommandList(m_CommandList);

		SK_CORE_TRACE_TAG("Renderer", "CommandBuffer open '{}'", m_Name);
		m_CommandList->open();
		m_CommandList->beginMarker(m_Name.c_str());
		//RT_BeginTimerQuery(m_TimerQuery);
	}

	void RenderCommandBuffer::RT_End()
	{
		SK_PROFILE_FUNCTION();

		uint32_t poolIndex = Renderer::RT_GetCurrentFrameIndex() % m_TimerQueryPools.size();
		auto& pool = m_TimerQueryPools[poolIndex];
		
		//RT_EndTimerQuery(m_TimerQuery);
		m_CommandList->endMarker();
		m_CommandList->close();
		SK_CORE_TRACE_TAG("Renderer", "CommandBuffer close '{}'", m_Name);

		Application::Get().GetDeviceManager()->UnlockCommandList(m_CommandList);
	}

	void RenderCommandBuffer::RT_Execute()
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("CommandBuffer Execute");
		SK_CORE_TRACE_TAG("Renderer", "CommandBuffer Execute '{}'", m_Name);

		auto deviceManager = Application::Get().GetDeviceManager();
		auto device = deviceManager->GetDevice();

		deviceManager->Lock();
		deviceManager->ExecuteCommandList(m_CommandList);

		uint32_t poolIndex = (Renderer::RT_GetCurrentFrameIndex() + 1) % m_TimerQueryPools.size();
		auto& pool = m_TimerQueryPools[poolIndex];

		for (uint32_t queryIndex = 0; queryIndex < pool.size(); queryIndex++)
		{
			nvrhi::ITimerQuery* query = pool.at(queryIndex).first;
			bool& started = pool.at(queryIndex).second;

			float time = 0.0f;
			if (started)
			{
				if (device->pollTimerQuery(query))
				{
					time = device->getTimerQueryTime(query);
					device->resetTimerQuery(query);
					started = false;
				}
				else
				{
					SK_CORE_DEBUG("Failed to retrive timer. Query: ({}, {}), Frame: {}", poolIndex, queryIndex, Renderer::RT_GetCurrentFrameIndex());
				}
			}

			m_GPUExecutionTimes[poolIndex][queryIndex] = time;
		}

		deviceManager->Unlock();
	}

	void RenderCommandBuffer::BeginMarker(const char* name)
	{
		Ref instance = this;
		Renderer::Submit([instance, n = std::string(name)]()
		{
			instance->m_CommandList->beginMarker(n.c_str());
		});
	}

	void RenderCommandBuffer::EndMarker()
	{
		Ref instance = this;
		Renderer::Submit([instance]()
		{
			instance->m_CommandList->endMarker();
		});
	}

	QueryID RenderCommandBuffer::RegisterTimerQuery()
	{
		uint32_t queryID = m_NextQueryID++;

		auto device = Application::Get().GetDeviceManager()->GetDevice();
		for (uint32_t poolIndex = 0; poolIndex < m_TimerQueryPools.size(); poolIndex++)
		{
			m_TimerQueryPools[poolIndex].emplace_back(device->createTimerQuery(), false);
			m_GPUExecutionTimes[poolIndex].emplace_back(0.0f);
		}

		return QueryID(queryID);
	}

	void RenderCommandBuffer::BeginTimerQuery(QueryID queryID)
	{
		SK_CORE_ASSERT(queryID.IsValid(), "Invalid QueryID {}. Call RegisterTimerQuery befor using it.", queryID.m_ID);

		Ref instance = this;
		Renderer::Submit([instance, queryID]()
		{
			instance->RT_BeginTimerQuery(queryID);
		});
	}

	void RenderCommandBuffer::EndTimerQuery(QueryID queryID)
	{
		SK_CORE_ASSERT(queryID.IsValid(), "Invalid QueryID {}. Call RegisterTimerQuery befor using it.", queryID.m_ID);

		Ref instance = this;
		Renderer::Submit([instance, queryID]()
		{
			instance->RT_EndTimerQuery(queryID);
		});
	}

	void RenderCommandBuffer::RT_BeginTimerQuery(QueryID queryID)
	{
		SK_CORE_ASSERT(queryID.IsValid(), "Invalid QueryID {}. Call RegisterTimerQuery befor using it.", queryID.m_ID);

		uint32_t poolIndex = Renderer::RT_GetCurrentFrameIndex() % m_TimerQueryPools.size();
		auto& pool = m_TimerQueryPools[poolIndex];

		auto& query = pool.at(queryID);
		m_CommandList->beginTimerQuery(query.first);
		query.second = true;
	}

	void RenderCommandBuffer::RT_EndTimerQuery(QueryID queryID)
	{
		SK_CORE_ASSERT(queryID.IsValid(), "Invalid QueryID {}. Call RegisterTimerQuery befor using it.", queryID.m_ID);

		uint32_t poolIndex = Renderer::RT_GetCurrentFrameIndex() % m_TimerQueryPools.size();
		auto& pool = m_TimerQueryPools[poolIndex];

		auto& query = pool.at(queryID);
		m_CommandList->endTimerQuery(query.first);
		query.second = true;
	}

	TimeStep RenderCommandBuffer::GetGPUExecutionTime(QueryID queryID) const
	{
		SK_CORE_ASSERT(queryID.IsValid(), "Invalid QueryID {}. Call RegisterTimerQuery befor using it.", queryID.m_ID);

		uint32_t frameIndex = Renderer::GetCurrentFrameIndex() % m_TimerQueryPools.size();
		return m_GPUExecutionTimes[frameIndex][queryID];
	}

	TimeStep RenderCommandBuffer::GetGPUExecutionTime(uint32_t frameIndex, QueryID queryID) const
	{
		SK_CORE_ASSERT(queryID.IsValid(), "Invalid QueryID {}. Call RegisterTimerQuery befor using it.", queryID.m_ID);

		frameIndex %= m_TimerQueryPools.size();
		return m_GPUExecutionTimes[frameIndex][queryID];
	}

}