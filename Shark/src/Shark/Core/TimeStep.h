#pragma once

namespace Shark {

	class TimeStep
	{
	public:
		TimeStep(float time = 0.0f)
			: m_Time(time)
		{}

		operator float() const { return m_Time; }

		float Seconds() const { return m_Time; }
		float MilliSeconds() const { return m_Time * 1000.0f; }
		float MicroSeconds() const { return m_Time * (1000.0f * 1000.0f); }
		float NanoSeconds() const { return m_Time * (1000.0f * 1000.0f * 1000.0f); }

		float s() const { return Seconds(); }
		float ms() const { return MilliSeconds(); }
		float us() const { return MicroSeconds(); }
		float ns() const { return NanoSeconds(); }

	public:
		TimeStep operator+(const TimeStep& rhs) const { return m_Time + rhs.m_Time; }
		TimeStep operator-(const TimeStep& rhs) const { return m_Time + rhs.m_Time; }

		TimeStep& operator+=(const TimeStep& rhs) { m_Time += rhs.m_Time; return *this; }
		TimeStep& operator-=(const TimeStep& rhs) { m_Time += rhs.m_Time; return *this; }

	private:
		float m_Time;
	};

}