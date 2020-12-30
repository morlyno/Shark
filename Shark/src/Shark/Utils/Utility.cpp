#include "skpch.h"
#include "Utility.h"

namespace Shark {

	ApplicationTime ApplicationTime::s_Instance;

	double ApplicationTime::GetSeconts()
	{
		auto time = std::chrono::steady_clock::now();
		std::chrono::duration<double> dur = time - s_Instance.m_TimerStart;
		return dur.count();
	}

	double ApplicationTime::GetMilliSeconts()
	{
		auto time = std::chrono::steady_clock::now();
		std::chrono::duration<double, std::milli> dur = time - s_Instance.m_TimerStart;
		return dur.count();
	}

	double Timer::End()
	{
		auto endTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> dur = endTime - m_Start;
		return dur.count();
	}

}