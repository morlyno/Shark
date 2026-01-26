#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	class RenderCommandBuffer : public RefCount
	{
	public:
		static Ref<RenderCommandBuffer> Create(const std::string& name) { return Ref<RenderCommandBuffer>::Create(name); }

	public:
		void Begin();
		void End();
		void Execute();

		void RT_Begin();
		void RT_End();
		void RT_Execute();

		nvrhi::CommandListHandle GetHandle() const { return m_CommandList; }
		nvrhi::GraphicsState& GetGraphicsState() { return m_GraphicsState; }
		nvrhi::ComputeState& GetComputeState() { return m_ComputeState; }

		void BeginMarker(const char* name);
		void EndMarker();

		void BeginTimer(std::string_view timerName);
		void EndTimer(std::string_view timerName);
		void RT_BeginTimer(std::string_view timerName);
		void RT_EndTimer(std::string_view timerName);

		TimeStep GetGPUExecutionTime() const;
		TimeStep GetGPUExecutionTime(std::string_view timerName) const;

		// #Renderer #Investigate Pipeline Statistics are not supported by nvrhi

	public:
		RenderCommandBuffer(const std::string& name);
		~RenderCommandBuffer();

	private:
		std::string m_Name;
		nvrhi::CommandListHandle m_CommandList;
		nvrhi::GraphicsState m_GraphicsState;
		nvrhi::ComputeState m_ComputeState;

		struct TimerQueries
		{
			nvrhi::TimerQueryHandle m_Timer;

			std::map<std::string, std::pair<nvrhi::TimerQueryHandle, bool>, std::ranges::less> m_TimerQuery;
		};

		std::array<TimerQueries, 3> m_Queries;

		float m_TimerResult[2];
		std::map<std::string_view, float> m_TimerResults[2];
	};

}
