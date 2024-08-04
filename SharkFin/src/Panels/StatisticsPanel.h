#pragma once

#include "Panel.h"

namespace Shark {

	class StatisticsPanel : public Panel
	{
	public:
		StatisticsPanel(const std::string& panelName);

		virtual void OnImGuiRender(bool& shown) override;
		virtual void SetContext(Ref<Scene> context) override { m_Scene = context; }

	private:
		void UI_Memory();
		void UI_Profiler();

	private:
		Ref<Scene> m_Scene;

		struct ProfilerEntry
		{
			std::string Name;
			TimeStep Time;
			TimeStep Min = FLT_MAX;
			TimeStep Max = FLT_MIN;
			float AvgSamples = 0.0f;
		};

		std::vector<ProfilerEntry> m_ProfilerStats;
		TimeStep m_FrameTime;
		TimeStep m_CPUTime;
		TimeStep m_GPUTime;
		TimeStep m_ImGuiGPUTime;

		struct AccumulatorEntry
		{
			TimeStep Time;
			TimeStep Min = FLT_MAX;
			TimeStep Max = FLT_MIN;
			uint32_t Samples = 0;
		};

		std::map<std::string, AccumulatorEntry> m_ProfilerStatsAccumulator;
		TimeStep m_FrameTimeAccumulator;
		TimeStep m_CPUTimeAccumulator;
		TimeStep m_GPUTimeAccumulator;
		TimeStep m_ImGuiGPUTimeAccumulator;

		uint32_t m_ProfilerSamples = 10;
		uint32_t m_ProfilerSampleCount = 0;

	};

}
