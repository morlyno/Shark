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
		static TimeStep GetTimeOf(const std::string& name);
		static void NewFrame();

		static const std::unordered_map<std::string, ProfilerInstance>& GetMap();

		static std::unordered_map<std::string, ProfilerInstance>::iterator begin();
		static std::unordered_map<std::string, ProfilerInstance>::iterator end();
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

		TimeStep GetAverage() const { return m_AverageDuration; }
	private:
		std::vector<double> m_Durations;
		double m_AverageDuration = 0;
		double m_StartTime = 0;
	};

}

#if SK_ENABLE_PREF
#define SK_PERF_NEW_FRAME() ::Shark::ProfilerRegistry::NewFrame()

#define SK_PERF_REGISTRY_MAP() ::Shark::ProfilerRegistry::GetMap()
#define SK_PERF_PROFILER(name) ::Shark::ProfilerRegistry::GetProfiler((name))
#define SK_PERF_TIME(name) ::Shark::ProfilerRegistry::GetTimeOf(name)

#define SK_PERF_SCOPED(name) ::Shark::ScopedProfiler SK_UNIQUE_VAR_NAME = SK_PERF_PROFILER(name)
#define SK_PERF_FUNCTION() ::Shark::ScopedProfiler SK_UNIQUE_VAR_NAME = SK_PERF_PROFILER(SK_FUNC_SIG)
#else
#define SK_PERF_NEW_FRAME()

#define SK_PERF_PROFILER(name)
#define SK_PERF_TIME(name)

#define SK_PERF_SCOPED(name)
#define SK_PERF_FUNCTION()
#endif
