#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"

#include <chrono>

namespace Shark {

	class TimeUtils
	{
	public:
		using DefaultClock = std::chrono::high_resolution_clock;
		using TimePoint = DefaultClock::time_point;

		static inline TimePoint CurrentTime()
		{
			return DefaultClock::now();
		}

		static inline TimeStep Now()
		{
			return TimeStep::FromNanoSeconds((float)CurrentTime().time_since_epoch().count());
		}

		static inline TimeStep GetTimeStep(DefaultClock::duration dur)
		{
			return TimeStep::FromNanoSeconds((float)dur.count());
		}

	};

}
