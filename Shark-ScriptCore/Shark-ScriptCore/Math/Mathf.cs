
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
			=> x0 + (x1 - x0) * t;

		public static float Pow2(float a)
			=> a * a;

		public static float Min(float a, float b)
			=> Math.Min(a, b);
		public static float Max(float a, float b)
			=> Math.Max(a, b);

		public static float Clamp(float v, float min, float max)
			=> Math.Max(Math.Min(v, max), min);

	}
}