#include "skpch.h"
#include "RenderCommandBuffer.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	RenderCommandBuffer::RenderCommandBuffer(const std::string& name)
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

		for (auto& queries : m_Queries)
			queries.m_Timer = device->createTimerQuery();
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

		uint32_t index = Renderer::RT_GetCurrentFrameIndex() % m_Queries.size();
		auto& queries = m_Queries[index];

		SK_CORE_TRACE_TAG("Renderer", "CommandBuffer open '{}'", m_Name);
		Renderer::GetDeviceManager()->GetCommandListMutex().lock();
		m_CommandList->open();
		m_CommandList->beginMarker(m_Name.c_str());
		m_CommandList->beginTimerQuery(queries.m_Timer);
	}

	void RenderCommandBuffer::RT_End()
	{
		SK_PROFILE_FUNCTION();

		uint32_t index = Renderer::RT_GetCurrentFrameIndex() % m_Queries.size();
		auto& queries = m_Queries[index];

		m_CommandList->endTimerQuery(queries.m_Timer);
		m_CommandList->endMarker();
		m_CommandList->close();
		Renderer::GetDeviceManager()->GetCommandListMutex().unlock();
		SK_CORE_TRACE_TAG("Renderer", "CommandBuffer close '{}'", m_Name);
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
		deviceManager->Unlock();


		deviceManager->GetCommandListMutex().lock();

		uint32_t index = (Renderer::RT_GetCurrentFrameIndex() + 1) % m_Queries.size();
		auto& queries = m_Queries[index];

		const uint32_t resultIndex = Renderer::RT_GetCurrentFrameIndex() % 2;
		{
			if (device->pollTimerQuery(queries.m_Timer))
			{
				m_TimerResult[resultIndex] = device->getTimerQueryTime(queries.m_Timer);
				device->resetTimerQuery(queries.m_Timer);
			}
			else
				m_TimerResult[resultIndex] = 0.0f;
		}

		for (auto& [key, timerData] : queries.m_TimerQuery)
		{
			auto& [timer, started] = timerData;
			float time = 0.0f;

			if (started && device->pollTimerQuery(timer))
			{
				time = device->getTimerQueryTime(timer);
				device->resetTimerQuery(timer);
				started = false;
			}

			m_TimerResults[resultIndex][key] = time;
		}
		deviceManager->GetCommandListMutex().unlock();

		//deviceManager->Unlock();
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

	void RenderCommandBuffer::BeginTimer(std::string_view timerName)
	{
		Ref instance = this;
		Renderer::Submit([instance, name = std::string(timerName)]()
		{
			instance->RT_BeginTimer(name);
		});
	}

	void RenderCommandBuffer::EndTimer(std::string_view timerName)
	{
		Ref instance = this;
		Renderer::Submit([instance, name = std::string(timerName)]()
		{
			instance->RT_EndTimer(name);
		});
	}

	void RenderCommandBuffer::RT_BeginTimer(std::string_view timerName)
	{
		uint32_t index = Renderer::RT_GetCurrentFrameIndex() % m_Queries.size();
		auto& queries = m_Queries[index];

		auto i = queries.m_TimerQuery.find(timerName);
		if (i == queries.m_TimerQuery.end())
		{
			auto device = Renderer::GetGraphicsDevice();
			i = queries.m_TimerQuery.emplace(std::string(timerName), std::pair{ device->createTimerQuery(), false }).first;
		}

		m_CommandList->beginTimerQuery(i->second.first);
		i->second.second = true;
	}

	void RenderCommandBuffer::RT_EndTimer(std::string_view timerName)
	{
		uint32_t index = Renderer::RT_GetCurrentFrameIndex() % m_Queries.size();
		auto& queries = m_Queries[index];

		const auto i = queries.m_TimerQuery.find(timerName);
		if (i == queries.m_TimerQuery.end())
			return;
		
		m_CommandList->endTimerQuery(i->second.first);
	}

	TimeStep RenderCommandBuffer::GetGPUExecutionTime() const
	{
		uint32_t resultIndex = Renderer::GetCurrentFrameIndex() % 2;
		return m_TimerResult[resultIndex];
	}

	TimeStep RenderCommandBuffer::GetGPUExecutionTime(std::string_view timerName) const
	{
		uint32_t resultIndex = Renderer::GetCurrentFrameIndex() % 2;
		if (m_TimerResults[resultIndex].contains(timerName))
			return m_TimerResults[resultIndex].at(timerName);
		return 0.0f;
	}

}
