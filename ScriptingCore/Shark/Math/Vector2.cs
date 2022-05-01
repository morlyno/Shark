
using System;
using System.Runtime.InteropServices;

namespace Shark
{

	[StructLayout(LayoutKind.Sequential)]
	public struct Vector2 : IFormattable
	{
		public float X;
		public float Y;

		static public Vector2 Zero => new Vector2(0.0f);
		static public Vector2 One => new Vector2(1.0f);

		public static Vector2 Right => new Vector2(1.0f, 0.0f);
		public static Vector2 Left => new Vector2(-1.0f, 0.0f);
		public static Vector2 Up => new Vector2(0.0f, 1.0f);
		public static Vector2 Down => new Vector2(0.0f, -1.0f);


		public Vector2(float xy)
		{
			X = xy;
			Y = xy;
		}
		public Vector2(float x, float y)
		{
			X = x;
			Y = y;
		}
		public Vector2(Vector2 v)
		{
			X = v.X;
			Y = v.Y;
		}

		public static Vector2 operator +(Vector2 lhs, Vector2 rhs)
		{
			return new Vector2(lhs.X + rhs.X, lhs.Y + rhs.Y);
		}
		public static Vector2 operator -(Vector2 lhs, Vector2 rhs)
		{
			return new Vector2(lhs.X - rhs.X, lhs.Y - rhs.Y);
		}
		public static Vector2 operator *(Vector2 lhs, Vector2 rhs)
		{
			return new Vector2(lhs.X * rhs.X, lhs.Y * rhs.Y);
		}
		public static Vector2 operator /(Vector2 lhs, Vector2 rhs)
		{
			return new Vector2(lhs.X / rhs.X, lhs.Y / rhs.Y);
		}
		public static Vector2 operator *(Vector2 lhs, float s)
		{
			return new Vector2(lhs.X * s, lhs.Y * s);
		}
		public static Vector2 operator /(Vector2 lhs, float s)
		{
			return new Vector2(lhs.X / s, lhs.Y / s);
		}
		public static Vector2 operator *(float s, Vector2 rhs)
		{
			return new Vector2(s * rhs.X, s * rhs.Y);
		}
		public static Vector2 operator /(float s, Vector2 rhs)
		{
			return new Vector2(s / rhs.X, s / rhs.Y);
		}
		public static Vector2 operator +(Vector2 v)
		{
			return new Vector2(v);
		}
		public static Vector2 operator -(Vector2 v)
		{
			return new Vector2(-v.X, -v.Y);
		}

		public float Dot(Vector2 other)
		{
			return Mathf.Dot(this, other);
		}

		public float Length()
		{
			return Mathf.Sqrt(X * Y);
		}

		public Vector2 Normalize()
		{
			float length = Length();
			if (length == 0)
			{
				return new Vector2(0.0f);
			}

			return this / length;
		}

		public static Vector2 Lerp(Vector2 p0, Vector2 p1, float t)
		{
			return new Vector2(
				p0.X + (p1.X - p0.X) * t,
				p0.Y + (p1.Y - p0.Y) * t
			);
		}

		public float this[int index]
		{
			get
			{
				if (index == 0)
				{
					return X;
				}
				else if (index == 1)
				{
					return Y;
				}
				else
				{
					throw new System.IndexOutOfRangeException($"Index between 0 and 1 expected but {index} provided");
				}
			}
			set
			{
				if (index == 0)
				{
					X = value;
				}
				else if (index == 1)
				{
					Y = value;
				}
				else
				{
					throw new System.IndexOutOfRangeException($"Index between 0 and 1 expected but {index} provided");
				}
			}
		}


		public string ToString(string format, IFormatProvider formatProvider)
		{
			return string.Format("[{0}, {1}]",
				X.ToString(format, formatProvider),
				Y.ToString(format, formatProvider)
			);
		}

	}

}