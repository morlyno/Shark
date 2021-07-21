#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"
#include <string>
#include <functional>

namespace Shark {

	// if time passed betwean calling update x time greater than time than the callback still gets called only once (timepassed is reseted fith TimePassed = fmod(Time, TimePassed)).
	class Counter
	{
	public:
		template<typename Func>
		static void Add(double time, bool repead, const Func& func) { Add("Unnamed", time, repead, std::function<void()>(func)); }
		template<typename Func>
		static void Add(const std::string& tag, double time, bool repead, const Func& func) { Add(tag, time, repead, std::function<void()>(func)); }

		static void Add(const std::string& tag, double time, bool repead, const std::function<void()>& func);

		static void SetActivce(const std::string& tag, bool active);
		static bool IsPaused(const std::string& tag);

		static void Pause(const std::string& tag) { SetActivce(tag, false); }
		static void Resume(const std::string& tag) { SetActivce(tag, true); }

		static double GetTime(const std::string& tag);
		static void SetTime(const std::string& tag, double time);

		static bool Remove(const std::string& tag);

		// Give This class its owne thread
		static void Update(TimeStep ts);
	};

}
