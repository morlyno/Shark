#pragma once

#include "Shark/Core/TimeStep.h"

#include <chrono>

namespace Shark {

	class Timer
	{
	public:
		using Clock = std::chrono::high_resolution_clock;

		Timer()
		{
			m_Start = Clock::now();
		}

		void Reset()
		{
			m_Start = Clock::now();
		}

		TimeStep Elapsed() const
		{
			return TimeStep::FromDuration(Clock::now() - m_Start);
		}

		float ElapsedMilliSeconds() const
		{
			return Elapsed().MilliSeconds();
		}

	private:
		Clock::time_point m_Start;
	};

	class ScopedTimer
	{
	public:
		ScopedTimer(const std::string& name)
			: m_Name(name)
		{
		}

		~ScopedTimer()
		{
			SK_CORE_TRACE_TAG("Timer", "{0} took {1}", m_Name, m_Timer.Elapsed());
		}

	private:
		Timer m_Timer;
		std::string m_Name;
	};

}
