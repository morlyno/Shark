#pragma once

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

		double Stop()
		{
			auto stop = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - m_Start);
			m_Stoped = true;
			return (double)duration.count();
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

}
