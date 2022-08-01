
using System;

namespace Shark
{
	public static class Mathf
	{

		public static float Abs(float val)
		{
			return (float)Math.Abs(val);
		}

		public static float Sqrt(float val)
		{
			return (float)Math.Sqrt((double)val);
		}

		public static float Lerp(float x0, float x1, float t)
		{
			return x0 + (x1 - x0) * t;
		}

	}
}