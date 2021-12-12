#pragma once

#include "Shark/Core/TimeStep.h"

namespace Shark {

	class Win32Timer
	{
	public:
		Win32Timer()
		{
			QueryPerformanceCounter(&m_Start);
		}

		~Win32Timer()
		{
			if (!m_Stoped)
				StopAndLog();
		}

		TimeStep Stop()
		{
			LARGE_INTEGER stop;
			QueryPerformanceCounter(&stop);
			LARGE_INTEGER frequncy;
			QueryPerformanceFrequency(&frequncy);

			auto duration = stop.QuadPart - m_Start.QuadPart;
			m_Stoped = true;
			return (double)duration / (double)frequncy.QuadPart;
		}

		void StopAndLog()
		{
			TimeStep time = Stop();
			SK_CORE_TRACE("{} ms", time.MilliSeconds());
		}

	private:
		LARGE_INTEGER m_Start;
		bool m_Stoped = false;
	};

	class CPPTimer
	{
	public:
		CPPTimer()
		{
			m_Start = std::chrono::high_resolution_clock::now();
		}

		~CPPTimer()
		{
			if (!m_Stoped)
				StopAndLog();
		}

		double Stop()
		{
			auto stop = std::chrono::high_resolution_clock::now();
			auto duration = stop - m_Start;
			m_Stoped = true;
			return (double)duration.count() * 0.001 * 0.001;
		}

		void StopAndLog()
		{
			double time = Stop();
			SK_CORE_TRACE("{} ms", time);
		}

	private:
		std::chrono::high_resolution_clock::time_point m_Start;
		bool m_Stoped = false;
	};

	using Timer = Win32Timer;

}
