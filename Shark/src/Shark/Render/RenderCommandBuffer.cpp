#include "skpch.h"
#include "RenderCommandBuffer.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	RenderCommandBuffer::RenderCommandBuffer(const std::string& name, bool enableQueries)
		: m_Name(name), m_EnableQueries(enableQueries), m_DoQuery(enableQueries)
	{
		auto deviceManager = Application::Get().GetDeviceManager();
		auto device = deviceManager->GetDevice();

		// NOTE(moro): nvrhi doesn't support deferred command lists for d3d11
		const bool deferredSupported = true;// device->queryFeatureSupport(nvrhi::Feature::DeferredCommandLists);

		auto desc = nvrhi::CommandListParameters()
			.setEnableImmediateExecution(!deferredSupported)
			.setQueueType(nvrhi::CommandQueue::Graphics);

		m_CommandList = device->createCommandList(desc);

		if (!m_EnableQueries)
			return;

		for (auto& queries : m_FrameTimer)
			queries = device->createTimerQuery();
	}

	RenderCommandBuffer::~RenderCommandBuffer()
	{
	}

	void RenderCommandBuffer::Begin()
	{
		SK_CORE_TRACE_TAG("Renderer", "CommandBuffer open '{}'", m_Name);

		Ref instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_Begin();
		});
	}

	void RenderCommandBuffer::End()
	{
		SK_CORE_TRACE_TAG("Renderer", "CommandBuffer close '{}'", m_Name);

		Ref instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_End();
		});
	}

	void RenderCommandBuffer::Execute()
	{
		SK_CORE_TRACE_TAG("Renderer", "CommandBuffer Execute '{}'", m_Name);

		Ref instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_Execute();
		});
	}

	void RenderCommandBuffer::RT_Begin()
	{
		SK_PROFILE_FUNCTION();

		const uint32_t index = Renderer::RT_GetCurrentFrameIndex();
		if (index == m_LastOpenFrame)
		{
			SK_CORE_WARN_TAG("Renderer", "[CommandBuffer '{}'] Opening more than once a frame can produce undefined behavior. Queries disabled for this frame", m_Name);
			m_DoQuery = false;
		}

		m_LastOpenFrame = index;

		SK_CORE_TRACE_TAG("Renderer", "[RT] CommandBuffer open '{}'", m_Name);
		m_CommandList->open();
		m_CommandList->beginMarker(m_Name.c_str());

		if (!m_DoQuery)
			return;

		m_CurrentTimer = m_FrameTimer[index % 3];
		m_CurrentStack = &m_NamedTimers[index % 3];

		const uint32_t resultIndex = Renderer::RT_GetCurrentFrameIndex() % 2;
		const uint32_t lastResultIndex = (Renderer::RT_GetCurrentFrameIndex() - 1) % 2;
		auto device = Renderer::GetGraphicsDevice();

		if (device->pollTimerQuery(m_CurrentTimer))
		{
			float seconds = device->getTimerQueryTime(m_CurrentTimer);
			m_TimerResult[resultIndex] = seconds;
		}

		device->resetTimerQuery(m_CurrentTimer);

		for (auto& [name, timerQuery] : *m_CurrentStack)
		{
			if (device->pollTimerQuery(timerQuery))
			{
				float seconds = device->getTimerQueryTime(timerQuery);

				if (!m_NamedResults[lastResultIndex].contains(name))
				{
					m_NamedResults[resultIndex][name] = seconds;
				}
				else
				{
					m_NamedResults[resultIndex][name] = m_NamedResults[lastResultIndex][name] * 0.2f + seconds * 0.8f;
				}

				device->resetTimerQuery(timerQuery);
			}
			else
			{
				SK_CORE_WARN_TAG("Renderer", "[CommandBuffer '{}'] [{}] Timer {} not ready", m_Name, Renderer::RT_GetCurrentFrameIndex(), name);
			}
		}

		m_CommandList->beginTimerQuery(m_CurrentTimer);
	}

	void RenderCommandBuffer::RT_End()
	{
		SK_PROFILE_FUNCTION();

		if (m_DoQuery)
		{
			m_CommandList->endTimerQuery(m_CurrentTimer);

			SK_CORE_ASSERT(m_TimerStack.empty());
		}

		m_CommandList->endMarker();
		m_CommandList->close();
		SK_CORE_TRACE_TAG("Renderer", "[RT] CommandBuffer close '{}'", m_Name);
	}

	void RenderCommandBuffer::RT_Execute()
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("CommandBuffer Execute");
		SK_CORE_TRACE_TAG("Renderer", "[RT] CommandBuffer Execute '{}'", m_Name);

		auto deviceManager = Application::Get().GetDeviceManager();
		auto device = deviceManager->GetDevice();

		deviceManager->LockQueue();
		deviceManager->ExecuteCommandList(m_CommandList);
		deviceManager->UnlockQueue();

		m_DoQuery = m_EnableQueries;
	}

	void RenderCommandBuffer::BeginMarker(const char* name)
	{
		Ref instance = this;
		Renderer::Submit([instance, n = std::string(name)]()
		{
			instance->m_MarkerStack /= n;
			instance->m_CommandList->beginMarker(n.c_str());
		});
	}

	void RenderCommandBuffer::EndMarker()
	{
		Ref instance = this;
		Renderer::Submit([instance]()
		{
			instance->m_CommandList->endMarker();
			instance->m_MarkerStack = instance->m_MarkerStack.parent_path();
		});
	}

	void RenderCommandBuffer::BeginTimer(std::string_view timerName)
	{
		SK_CORE_ASSERT(m_EnableQueries);
		SK_CORE_ASSERT(timerName.find_first_of("/\\") == std::string_view::npos);

		Ref instance = this;
		Renderer::Submit([instance, name = std::string(timerName)]()
		{
			instance->RT_BeginTimer(name);
		});
	}

	void RenderCommandBuffer::EndTimer()
	{
		SK_CORE_ASSERT(m_EnableQueries);

		Ref instance = this;
		Renderer::Submit([instance]()
		{
			instance->RT_EndTimer();
		});
	}

	void RenderCommandBuffer::RT_BeginTimer(std::string_view timerName)
	{
		SK_CORE_ASSERT(m_EnableQueries);
		SK_CORE_ASSERT(timerName.find_first_of("/\\") == std::string_view::npos);
		SK_CORE_ASSERT(!timerName.empty());

		if (!m_DoQuery)
			return;

		m_LastBegin = timerName;
		auto i = m_CurrentStack->find(timerName);
		if (i == m_CurrentStack->end())
		{
			auto device = Renderer::GetGraphicsDevice();
			i = m_CurrentStack->emplace(m_LastBegin, device->createTimerQuery()).first;
		}

		m_TimerStack /= m_LastBegin;
		m_CommandList->beginTimerQuery(i->second);
	}

	void RenderCommandBuffer::RT_EndTimer()
	{
		SK_CORE_ASSERT(m_EnableQueries);

		if (!m_DoQuery)
			return;

		const auto i = m_CurrentStack->find(m_LastBegin);
		if (i == m_CurrentStack->end())
			return;

		m_TimerStack = m_TimerStack.parent_path();
		m_LastBegin = m_TimerStack.filename().string();
		m_CommandList->endTimerQuery(i->second);
	}

	TimeStep RenderCommandBuffer::GetGPUExecutionTime() const
	{
		uint32_t resultIndex = Renderer::GetCurrentFrameIndex() % 2;
		return m_TimerResult[resultIndex];
	}

	TimeStep RenderCommandBuffer::GetGPUExecutionTime(std::string_view timerName) const
	{
		uint32_t resultIndex = Renderer::GetCurrentFrameIndex() % 2;
		if (m_NamedResults[resultIndex].contains(timerName))
			return m_NamedResults[resultIndex].at(timerName);
		return 0.0f;
	}

}
