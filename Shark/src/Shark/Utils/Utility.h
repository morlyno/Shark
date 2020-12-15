#pragma once

namespace Shark {

	namespace Color {
		struct F32RGBA
		{
			union
			{
				struct { float r,g,b,a; };
				float rgba[4];
			};
			F32RGBA( float r,float g,float b,float a = 1.0f )
				: r( r ),g( g ),b( b ),a( a )
			{}
		};
	}

	class ApplicationTime
	{
	public:
		// returns Time since start of Application
		static double GetSeconts();
		static double GetMilliSeconts();
	private:
		static ApplicationTime s_Instance;
		const std::chrono::steady_clock::time_point m_TimerStart = std::chrono::steady_clock::now();
	};

	// For counting Time from Creation until function End is called;
	class Timer
	{
	public:
		Timer() = default;
		double End();
	private:
		const std::chrono::steady_clock::time_point m_Start = std::chrono::steady_clock::now();
	};

}