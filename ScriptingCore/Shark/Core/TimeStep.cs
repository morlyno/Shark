
namespace Shark
{

	public struct TimeStep
	{
		private float m_Time;

		public TimeStep(float time = 0.0f)
		{
			m_Time = time;
		}

		public float Seconds => m_Time;
		public float MilliSeconds => m_Time * 1000.0f;
		public float MicroSeconds => m_Time * (1000.0f * 1000.0f);
		public float NanoSeconds => m_Time * (1000.0f * 1000.0f * 1000.0f);

		public static TimeStep operator+(TimeStep lhs, TimeStep rhs) { return new TimeStep(lhs.m_Time + rhs.m_Time); }
		public static TimeStep operator-(TimeStep lhs, TimeStep rhs) { return new TimeStep(lhs.m_Time - rhs.m_Time); }


		public static explicit operator float(TimeStep This) { return This.m_Time; }
	}

}