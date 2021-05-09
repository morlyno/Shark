#pragma once

#include <iostream>

namespace Shark {

	class Timer
	{
	public:
		Timer()
		{
			m_Start = std::chrono::high_resolution_clock::now();
		}

		~Timer()
		{
			if (!m_Stoped)
				StopAndLog();
		}

		float Stop()
		{
			auto stop = std::chrono::high_resolution_clock::now();
			auto duration = stop - m_Start;
			m_Stoped = true;
			return duration.count() * 0.001f * 0.001f;
		}

		void StopAndLog()
		{
			std::cout << Stop() << " ms" << std::endl;
		}

	private:
		std::chrono::high_resolution_clock::time_point m_Start;
		bool m_Stoped = false;
	};

}
