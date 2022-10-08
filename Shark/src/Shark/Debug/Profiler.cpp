#include "skpch.h"
#include "Profiler.h"

#include "Shark\Utils\TimeUtils.h"

namespace Shark {

	PerformanceProfiler* PerformanceProfiler::s_Instance = nullptr;

	void PerformanceProfiler::Initialize()
	{
		SK_CORE_ASSERT(!s_Instance);
		s_Instance = new PerformanceProfiler();
		s_Instance->ActiveStorage = &s_Instance->TimerStorageBuffers[0];
		s_Instance->FinishedSotrage = &s_Instance->TimerStorageBuffers[1];
	}

	void PerformanceProfiler::Shutdown()
	{
		SK_CORE_ASSERT(s_Instance);
		delete s_Instance;
		s_Instance = nullptr;
	}

	void PerformanceProfiler::SetSampleRate(uint32_t count)
	{
		s_Instance->SampleRate = count;
	}

	uint32_t PerformanceProfiler::GetSampleRate()
	{
		return s_Instance->SampleRate;
	}

	void PerformanceProfiler::NewFrame()
	{
		if ((++s_Instance->Frame % s_Instance->SampleRate) == 0)
		{
			std::swap(s_Instance->ActiveStorage, s_Instance->FinishedSotrage);
			s_Instance->ActiveStorage->clear();

			if (s_Instance->SampleRate > 1)
			{
				for (auto& [name, time] : *s_Instance->FinishedSotrage)
					time /= (float)s_Instance->SampleRate;
			}

		}

		float time = TimeUtils::Now();
		float dur = time - s_Instance->FrameStartTime;
		Push("Frame", dur);
		s_Instance->FrameStartTime = time;
	}

	void PerformanceProfiler::Push(std::string_view key, float time)
	{
		auto& storage = s_Instance->GetStorage();
		storage[key] += time;
	}

	const PerformanceProfiler::TimerStorage& PerformanceProfiler::GetStatistics()
	{
		return *s_Instance->FinishedSotrage;
	}

	PerformanceProfiler::TimerStorage& PerformanceProfiler::GetStorage()
	{
		return *s_Instance->ActiveStorage;
	}


	PerformanceTimer::PerformanceTimer(std::string_view key)
		: m_Key(key)
	{
		Start();
	}

	PerformanceTimer::~PerformanceTimer()
	{
		Stop();
	}

	void PerformanceTimer::Start()
	{
		m_Start = TimeUtils::Now();
	}

	void PerformanceTimer::Stop()
	{
		float stop = TimeUtils::Now();
		float dur = stop - m_Start;
		PerformanceProfiler::Push(m_Key, dur);
	}

}
