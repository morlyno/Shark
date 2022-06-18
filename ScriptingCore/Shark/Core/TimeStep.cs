
using System;

namespace Shark
{

	public struct TimeStep : IFormattable
	{
		private float m_Time;

		public TimeStep(float time = 0.0f)
		{
			m_Time = time;
		}

		public static TimeStep Sec(float val) => new TimeStep(val);
		public static TimeStep MilliSec(float val) => new TimeStep(val * 0.001f);
		public static TimeStep MicroSec(float val) => new TimeStep(val * (0.001f * 0.001f));
		public static TimeStep NanoSec(float val) => new TimeStep(val * (0.001f * 0.001f * 0.001f));

		public float Seconds => m_Time;
		public float MilliSeconds => m_Time * 1000.0f;
		public float MicroSeconds => m_Time * (1000.0f * 1000.0f);
		public float NanoSeconds => m_Time * (1000.0f * 1000.0f * 1000.0f);

		public float Time { get { return m_Time; } set { m_Time = value; } }

		public void Reset() { m_Time = 0.0f; }

		public static TimeStep operator+(TimeStep lhs, TimeStep rhs) { return new TimeStep(lhs.m_Time + rhs.m_Time); }
		public static TimeStep operator-(TimeStep lhs, TimeStep rhs) { return new TimeStep(lhs.m_Time - rhs.m_Time); }


		public static implicit operator TimeStep(float f) { return new TimeStep(f); }
		public static implicit operator float(TimeStep This) { return This.m_Time; }

		public override string ToString()
		{
			return ToString(null, null);
		}
		public string ToString(string format)
		{
			return ToString(format, null);
		}
		public string ToString(string format, IFormatProvider formatProvider)
		{
			return m_Time.ToString(format, formatProvider);
		}
	}

}