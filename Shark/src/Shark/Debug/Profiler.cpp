#include "skpch.h"
#include "Profiler.h"

#include "Shark/Utility/Utility.h"

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

		static double CurrentTime()
		{
			return (double)TickCount() / (double)s_ProfilingData->CPUFrequency;
		}

	}


	ProfilerInstance& ProfilerRegistry::GetProfiler(const std::string& name)
	{
		return s_ProfilingData->Registry[name];
	}


	TimeStep ProfilerRegistry::GetTimeOf(const std::string& name)
	{
		if (!Utility::Contains(s_ProfilingData->Registry, name))
			return 0.0f;
		const ProfilerInstance& profiler = s_ProfilingData->Registry.at(name);
		return profiler.GetAverage();
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

	std::unordered_map<std::string, ProfilerInstance>::iterator ProfilerRegistry::begin()
	{
		return s_ProfilingData->Registry.begin();
	}

	std::unordered_map<std::string, ProfilerInstance>::iterator ProfilerRegistry::end()
	{
		return s_ProfilingData->Registry.end();
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
		m_StartTime = Utils::CurrentTime();
	}

	void ProfilerInstance::EndTimer()
	{
		double endTime = Utils::CurrentTime();
		m_Durations.push_back(endTime - m_StartTime);
	}

	void ProfilerInstance::NewFrame()
	{
		m_AverageDuration = 0.0f;
		if (m_Durations.size() == 0)
			return;

		for (auto duration : m_Durations)
			m_AverageDuration += duration;
		m_AverageDuration /= m_Durations.size();
		m_Durations.clear();
	}

}
