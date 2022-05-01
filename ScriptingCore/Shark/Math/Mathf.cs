
using System;

namespace Shark
{
	public static class Mathf
	{
		public static Vector3 Cross(Vector3 x, Vector3 y)
		{
			return new Vector3(
				x.Y * y.Z - y.Y * x.Z,
				x.Z * y.X - y.Z * x.X,
				x.X * y.Y - y.X * x.Y
			);
		}

		public static float Dot(Vector2 x, Vector2 y)
		{
			Vector2 temp = x * y;
			return temp.X + temp.Y;
		}

		public static float Dot(Vector3 x, Vector3 y)
		{
			Vector3 temp = x * y;
			return temp.X + temp.Y + temp.Z;
		}

		public static float Dot(Vector4 x, Vector4 y)
		{
			Vector4 temp = x * y;
			return temp.X + temp.Y + temp.Z * temp.W;
		}

		public static float Sqrt(float val)
		{
			return (float)Math.Sqrt((double)val);
		}

		public static float Lerp(float x0, float x1, float t)
		{
			return x0 + (x1 - x0) * t;
		}

		public static Vector2 Sqrt(Vector2 v)
		{
			return new Vector2(
				(float)Math.Sqrt(v.X),
				(float)Math.Sqrt(v.Y)
			);
		}

		public static Vector3 Sqrt(Vector3 v)
		{
			return new Vector3(
				(float)Math.Sqrt(v.X),
				(float)Math.Sqrt(v.Y),
				(float)Math.Sqrt(v.Z)
			);
		}

		public static Vector4 Sqrt(Vector4 v)
		{
			return new Vector4(
				(float)Math.Sqrt(v.X),
				(float)Math.Sqrt(v.Y),
				(float)Math.Sqrt(v.Z),
				(float)Math.Sqrt(v.W)
			);
		}

	}
}