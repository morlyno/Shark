#pragma once

#include "Shark/Core/Concepts.h"
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

	template<string_like String>
	class ScopedTimer
	{
	public:
		ScopedTimer(String name)
			: m_Name(std::move(name)) {}

		ScopedTimer(Log::Tag tag, String name)
			: m_Tag(tag), m_Name(std::move(name)) {}

		ScopedTimer(Log::Level level, Log::Tag tag, String name)
			: m_Level(level), m_Tag(tag), m_Name(std::move(name)) {}

		~ScopedTimer()
		{
			Log::Get()->CoreLog(m_Level, m_Tag, "{} took {}", m_Name, m_Timer.Elapsed());
		}

	private:
		Log::Tag m_Tag = Log::Tag::Timer;
		Log::Level m_Level = Log::Level::Trace;

		Timer m_Timer;
		String m_Name;
	};

}
