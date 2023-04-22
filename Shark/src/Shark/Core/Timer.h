#pragma once

#include "Shark/Core/TimeStep.h"

#include <chrono>

#define SK_SCOPED_TIMER(_name) ::Shark::ScopedTimer SK_UNIQUE_NAME (_name)

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

		TimeStep Elapsed()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - m_Start).count() * 0.001f * 0.001f * 0.001f;
		}

		float ElapsedMilliSeconds()
		{
			return Elapsed().MilliSeconds();
		}

	private:
		Clock::time_point m_Start;
	};

	class ScopedTimer
	{
	public:
		ScopedTimer(std::string_view name)
			: m_Name(name)
		{
		}

		~ScopedTimer()
		{
			const float millis = m_Timer.ElapsedMilliSeconds();
			SK_CORE_TRACE_TAG("Timer", "{0} took {1}ms", m_Name, millis);
		}

	private:
		Timer m_Timer;
		std::string_view m_Name;
	};

}
