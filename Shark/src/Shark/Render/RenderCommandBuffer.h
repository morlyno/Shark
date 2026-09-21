#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	class RenderCommandBuffer : public RefCount
	{
	public:
		static Ref<RenderCommandBuffer> Create(std::string name, bool enableQueries = false) { return Ref<RenderCommandBuffer>::Create(std::move(name), enableQueries); }
		static Ref<RenderCommandBuffer> Create(nvrhi::CommandQueue queueType, std::string name, bool enableQueries = false) { return Ref<RenderCommandBuffer>::Create(queueType, std::move(name), enableQueries); }

	public:
		void Begin();
		void End();
		void Execute();

		void RT_Begin();
		void RT_End();
		void RT_Execute();

		nvrhi::CommandListHandle GetHandle() const { return m_CommandList; }
		nvrhi::CommandQueue GetQueueType() const { return m_QueueType; }
		nvrhi::GraphicsState& GetGraphicsState() { return m_GraphicsState; }
		nvrhi::ComputeState& GetComputeState() { return m_ComputeState; }

		void BeginMarker(const char* name);
		void EndMarker();

		void BeginTimer(std::string_view timerName);
		void EndTimer();
		void RT_BeginTimer(std::string_view timerName);
		void RT_EndTimer();

		TimeStep GetGPUExecutionTime() const;
		TimeStep GetGPUExecutionTime(std::string_view timerName) const;

		// #Renderer #Investigate Pipeline Statistics are not supported by nvrhi

	public:
		RenderCommandBuffer(std::string name, bool enableQueries);
		RenderCommandBuffer(nvrhi::CommandQueue queueType, std::string name, bool enableQueries);
		~RenderCommandBuffer();

	private:
		std::string m_Name;
		nvrhi::CommandQueue m_QueueType;
		bool m_EnableQueries;

		bool m_DoQuery = false;
		uint32_t m_LastOpenFrame = ~0u;

		nvrhi::CommandListHandle m_CommandList;
		nvrhi::GraphicsState m_GraphicsState;
		nvrhi::ComputeState m_ComputeState;

		using TimerMap = std::map<std::string, nvrhi::TimerQueryHandle, std::ranges::less>;

		std::filesystem::path m_TimerStack;
		std::string m_LastBegin;

		nvrhi::TimerQueryHandle m_CurrentTimer;
		TimerMap* m_CurrentStack = nullptr;

		std::array<nvrhi::TimerQueryHandle, 3> m_FrameTimer;
		std::array<TimerMap, 3> m_NamedTimers;

		float m_TimerResult[2];
		std::map<std::string_view, float> m_NamedResults[2];

		// Debug
		std::filesystem::path m_MarkerStack;
	};

	class ScopedGpuTimer
	{
	public:
		ScopedGpuTimer(RefArg<RenderCommandBuffer> commandBuffer, std::string_view timerName);
		~ScopedGpuTimer();

	private:
		Ref<RenderCommandBuffer> m_CommandBuffer;
	};

	class EventQuery : public RefCount
	{
	public:
		static Ref<EventQuery> Create() { return Ref<EventQuery>::Create(); }

	public:
		void RT_Set(nvrhi::CommandQueue queueType);
		void RT_Reset();

		bool RT_Poll();
		void RT_Wait();

		nvrhi::EventQueryHandle GetHandle() const { return m_EventQuery; }

	public:
		EventQuery();
		~EventQuery();

	private:
		nvrhi::EventQueryHandle m_EventQuery;
	};

}
