#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"
#include <string>
#include <functional>

namespace Shark {

	class Counter
	{
	public:
		template<typename Func>
		static void Add(double time, bool repead, const Func& func) { Add("Unnamed", time, repead, std::function<void()>(func)); }
		template<typename Func>
		static void Add(const std::string& tag, double time, bool repead, const Func& func) { Add(tag, time, repead, std::function<void()>(func)); }

		static void Add(const std::string& tag, double time, bool repead, const std::function<void()>& func);

		static void SetPause(const std::string& tag, bool paused);
		static bool IsPaused(const std::string& tag);

		static void Pause(const std::string& tag) { SetPause(tag, true); }
		static void Resume(const std::string& tag) { SetPause(tag, false); }

		static bool Remove(const std::string& tag);

		// Give This class its owne thread
		static void Update(TimeStep ts);
	};

}
