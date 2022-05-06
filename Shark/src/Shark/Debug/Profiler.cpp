#include "skpch.h"
#include "Profiler.h"

#include <Windows.h>

namespace Shark {

	struct ProfilingData
	{
		std::unordered_map<std::string, ProfilerInstance> Registry;
		uint64_t CPUFrequency = 0;
	};
	static Scope<ProfilingData> s_ProfilingData = nullptr;

	namespace Utils {

		static uint64_t TickCount()
		{
			LARGE_INTEGER ticks;
			bool result = QueryPerformanceCounter(&ticks);
			SK_CORE_ASSERT(result);
			return ticks.QuadPart;
		}

		static uint64_t Frequency()
		{
			LARGE_INTEGER frequency;
			bool result = QueryPerformanceFrequency(&frequency);
			SK_CORE_ASSERT(result);
			return frequency.QuadPart;
		}

	}


	ProfilerInstance& ProfilerRegistry::GetProfiler(const std::string& name)
	{
		return s_ProfilingData->Registry[name];
	}


	TimeStep ProfilerRegistry::GetAverageOf(const std::string& name)
	{
		if (s_ProfilingData->Registry.find(name) == s_ProfilingData->Registry.end())
			return 0.0f;
		const ProfilerInstance& profiler = s_ProfilingData->Registry.at(name);
		return profiler.GetAverage();
	}

	TimeStep ProfilerRegistry::GetTotalOf(const std::string& name)
	{
		if (s_ProfilingData->Registry.find(name) == s_ProfilingData->Registry.end())
			return 0.0f;
		const ProfilerInstance& profiler = s_ProfilingData->Registry.at(name);
		return profiler.GetTotal();
	}

	void ProfilerRegistry::NewFrame()
	{
		if (!s_ProfilingData)
			s_ProfilingData = Scope<ProfilingData>::Create();

		s_ProfilingData->CPUFrequency = Utils::Frequency();
		for (auto&& [name, profiler] : s_ProfilingData->Registry)
			profiler.NewFrame();
	}

	const std::unordered_map<std::string, ProfilerInstance>& ProfilerRegistry::GetMap()
	{
		return s_ProfilingData->Registry;
	}


	ScopedProfiler::ScopedProfiler(ProfilerInstance& profiler)
		: m_Profiler(profiler)
	{
		profiler.StartTimer();
	}

	ScopedProfiler::~ScopedProfiler()
	{
		m_Profiler.EndTimer();
	}


	void ProfilerInstance::StartTimer()
	{
		m_StartTime = Utils::TickCount();
	}

	void ProfilerInstance::EndTimer()
	{
		auto endTime = Utils::TickCount();
		m_Durations.push_back(endTime - m_StartTime);
		m_Frequency = s_ProfilingData->CPUFrequency;
	}

	void ProfilerInstance::NewFrame()
	{
		m_AverageDuration = 0;
		m_TotalDuration = 0;
		if (m_Durations.size() == 0)
			return;

		for (auto duration : m_Durations)
			m_TotalDuration += duration;
		m_AverageDuration = m_TotalDuration / m_Durations.size();
		m_Durations.clear();
	}

}
