#pragma once

#include "Panel.h"

namespace Shark {

	class StatisticsPanel : public Panel
	{
	public:
		StatisticsPanel();

		virtual void OnImGuiRender(bool& shown) override;
		virtual void SetContext(Ref<Scene> context) override { m_Scene = context; }

		static const char* GetStaticID() { return "StatisticsPanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }
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

		std::array<ProfilerEntry, 3> m_ApplicationStats;
		std::vector<ProfilerEntry> m_ProfilerStats;
		std::array<ProfilerEntry, 9> m_PhysicsStats;
		TimeStep m_FrameTime;
		TimeStep m_CPUTime;
		TimeStep m_ImGuiGPUTime;

		struct AccumulatorEntry
		{
			TimeStep Time;
			TimeStep Min = FLT_MAX;
			TimeStep Max = FLT_MIN;
			uint32_t Samples = 0;
		};

		std::array<AccumulatorEntry, 3> m_ApplicationAccumulators;
		std::map<std::string, AccumulatorEntry> m_ProfilerStatsAccumulator;
		std::array<AccumulatorEntry, 9> m_PhysicsAccumulators;

		uint32_t m_ProfilerSamples = 10;
		uint32_t m_ProfilerSampleCount = 0;

	};

}
