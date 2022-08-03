
using System;

namespace Shark
{
	public static class Mathf
	{

		public static float Abs(float val)
			=> (float)Math.Abs(val);

		public static float Sqrt(float val)
			=> (float)Math.Sqrt((double)val);

		public static float Lerp(float x0, float x1, float t)
		{
			return x0 + (x1 - x0) * t;
		}

		public static float Min(float a, float b)
			=> Math.Min(a, b);

	}
}