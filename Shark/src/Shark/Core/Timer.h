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
			: m_Tag("Timer"), m_Name(name) {}

		ScopedTimer(std::string_view tag, const std::string& name)
			: m_Tag(tag), m_Name(name) {}

		ScopedTimer(LogLevel level, std::string_view tag, const std::string& name)
			: m_Level(level), m_Tag(tag), m_Name(name) {}

		~ScopedTimer()
		{
			Log::PrintMessageTag(LoggerType::Core, m_Level, m_Tag, "{} took {}", m_Name, m_Timer.Elapsed());
		}

	private:
		Timer m_Timer;
		LogLevel m_Level = LogLevel::Trace;
		std::string_view m_Tag;
		std::string m_Name;
	};

}
