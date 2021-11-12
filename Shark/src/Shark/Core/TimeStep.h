#pragma once

namespace Shark {

	class TimeStep
	{
	public:
		TimeStep(double time = 0.0f)
			:
			m_Time(time)
		{}

		//operator float() const { return (float)m_Time; }
		operator double() const { return m_Time; }

		double Seconds() const { return m_Time; }
		double MilliSeconds() const { return m_Time * 1000.0; }
		double MicroSeconds() const { return m_Time * (1000.0 * 1000.0); }
		double NanoSeconds() const { return m_Time * (1000.0 * 1000.0 * 1000.0); }

		double s() const { return Seconds(); }
		double ms() const { return MilliSeconds(); }
		double us() const { return MicroSeconds(); }
		double ns() const { return NanoSeconds(); }

	private:
		double m_Time;
	};

}