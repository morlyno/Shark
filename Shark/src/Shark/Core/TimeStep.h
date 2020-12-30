#pragma once

namespace Shark {

	class TimeStep
	{
	public:
		TimeStep(float time = 0.0f)
			:
			m_Time(time)
		{}

		operator float() const { return m_Time; }

		float GetSeconts() { return m_Time; }
		float GetMilliSeconts() { return m_Time * 1000.0f; }

	private:
		const float m_Time;
	};

}