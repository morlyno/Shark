#include "skpch.h"
#include "Profiler.h"

namespace Shark {

	void PerformanceProfiler::Clear()
	{
		m_FrameStorage.clear();
	}

	void PerformanceProfiler::Add(std::string_view descriptor, float duration)
	{
		FrameData* storage = GetStorage(descriptor);
		storage->Descriptor = descriptor;
		storage->Duration += duration;
	}

	FrameData* PerformanceProfiler::GetStorage(std::string_view descriptor)
	{
		return &m_FrameStorage[descriptor];
	}

	ProfilerEvent::ProfilerEvent(PerformanceProfiler* profiler, std::string_view descriptor)
	{
		if (profiler)
		{
			Storage = profiler->GetStorage(descriptor);
			Storage->Descriptor = descriptor;
			m_Start = Platform::GetTicks();
		}
	}

	ProfilerEvent::~ProfilerEvent()
	{
		if (Storage)
		{
			m_Stop = Platform::GetTicks();
			Storage->Duration += (float)(m_Stop - m_Start) / (float)Platform::GetTicksPerSecond();
		}
	}

}
