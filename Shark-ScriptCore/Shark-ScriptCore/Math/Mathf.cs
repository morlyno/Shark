
using System;

namespace Shark
{
	public static class Mathf
	{
		public static readonly float Deg2Rad = 0.01745329251994329576923690768489f;
		public static readonly float Rad2Deg = 57.295779513082320876798154814105f;

		public static float Abs(float val)
			=> (float)System.Math.Abs(val);

		public static float Sqrt(float val)
			=> (float)System.Math.Sqrt((double)val);

		public static float Lerp(float x0, float x1, float t)
			=> x0 + (x1 - x0) * t;

		public static float Pow2(float a)
			=> a * a;

		public static float Min(float a, float b)
			=> System.Math.Min(a, b);
		public static float Max(float a, float b)
			=> System.Math.Max(a, b);

		public static float Clamp(float v, float min, float max)
			=> System.Math.Max(System.Math.Min(v, max), min);

	}

	public static class Math
	{
		public static int Max(int a, int b)
			=> a > b ? a : b;

		public static int Min(int a, int b)
			=> a < b ? a : b;

		public static int Clamp(int value, int min, int max)
			=> Max(Min(value, max), min);
	}

}