#include "skpch.h"
#include "Profiler.h"

#include "Shark/Core/Application.h"
#include "Shark/Utils/PlatformUtils.h"

namespace Shark {

	void PerformanceProfiler::Clear()
	{
		m_FrameStorage.clear();
	}

	void PerformanceProfiler::AddTiming(std::string_view name, float time)
	{
		if (!m_FrameStorage.contains(name))
			m_FrameStorage[name] = { 0.0f, 0 };

		auto& perFrameData = m_FrameStorage.at(name);
		perFrameData.Time += time;
		perFrameData.Samples++;
	}

	ProfilerEvent::ProfilerEvent(PerformanceProfiler* profiler, std::string_view name)
	{
		if (!profiler)
			return;

		m_Profiler = profiler;
		m_Name = name;
		m_StartTime = Platform::GetTicks();
	}

	ProfilerEvent::ProfilerEvent(std::string_view name)
		: ProfilerEvent(Application::Get().GetProfiler(), name)
	{
	}

	ProfilerEvent::~ProfilerEvent()
	{
		if (!m_Profiler)
			return;

		uint64_t stopTime = Platform::GetTicks();
		float time = (float)(stopTime - m_StartTime) / (float)Platform::GetTicksPerSecond();
		m_Profiler->AddTiming(m_Name, time);
	}

}
