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

}
