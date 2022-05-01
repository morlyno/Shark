#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"

#include <queue>

namespace Shark {

	class ProfilerInstance;

	class ProfilerRegistry
	{
	public:
		static ProfilerInstance& GetProfiler(const std::string& name);
		static TimeStep GetAverageOf(const std::string& name);
		static TimeStep GetTotalOf(const std::string& name);
		static void NewFrame();

		static const std::unordered_map<std::string, ProfilerInstance>& GetMap();
	};

	class ScopedProfiler
	{
	public:
		ScopedProfiler(ProfilerInstance& profiler);

		~ScopedProfiler();

	private:
		ProfilerInstance& m_Profiler;
	};

	class ProfilerInstance
	{
	public:
		void StartTimer();
		void EndTimer();
		void NewFrame();

		void AddDuration(uint64_t tickCount) { m_Durations.push_back(tickCount); }
		void SetFrequency(uint64_t frequency) { m_Frequency = frequency; }

		TimeStep GetAverage() const { return (float)m_AverageDuration / (float)m_Frequency; }
		TimeStep GetTotal() const { return (float)m_TotalDuration / (float)m_Frequency; }
	private:
		std::vector<uint64_t> m_Durations;
		uint64_t m_AverageDuration = 0;
		uint64_t m_TotalDuration = 0;
		uint64_t m_StartTime = 0;
		uint64_t m_Frequency = 0;
	};

}

#if SK_ENABLE_PERF
#define SK_PERF_NEW_FRAME() ::Shark::ProfilerRegistry::NewFrame()

#define SK_PERF_REGISTRY_MAP() ::Shark::ProfilerRegistry::GetMap()
#define SK_PERF_PROFILER(name) ::Shark::ProfilerRegistry::GetProfiler((name))
#define SK_PERF_ADD_DURATION(name, duration) ::Shark::ProfilerRegistry::GetProfiler((name)).AddDuration((duration))
#define SK_PERF_SET_FREQUENCY(name, frequency) ::Shark::ProfilerRegistry::GetProfiler((name)).SetFrequency((frequency))

#define SK_PERF_SCOPED(name) ::Shark::ScopedProfiler SK_UNIQUE_VAR_NAME = SK_PERF_PROFILER(name)
#else
#define SK_PERF_NEW_FRAME()

#define SK_PERF_REGISTRY_MAP()
#define SK_PERF_PROFILER(name)
#define SK_PERF_ADD_DURATION(name, duration)

#define SK_PERF_SCOPED(name)
#endif
