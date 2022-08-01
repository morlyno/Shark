
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


		public static Vector2 Lerp(Vector2 p0, Vector2 p1, float t)
		{
			return new Vector2(
				p0.X + (p1.X - p0.X) * t,
				p0.Y + (p1.Y - p0.Y) * t
			);
		}
		public static float Dot(Vector2 x, Vector2 y)
		{
			Vector2 temp = x * y;
			return temp.X + temp.Y;
		}
		public static Vector2 Sqrt(Vector2 v)
		{
			return new Vector2(
				Mathf.Sqrt(v.X),
				Mathf.Sqrt(v.Y)
			);
		}
		public static Vector2 Normalize(Vector2 vec)
		{
			float length = vec.Length;
			if (length == 0)
			{
				return Zero;
			}
			return vec / length;
		}


		public float Length
		{
			get => Mathf.Sqrt(Dot(this, this));
		}
		public void Normalize()
		{
			float length = Length;
			if (length != 0)
				this = this / length;
			else
				this = Zero;
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
			return string.Format("[{0}, {1}]",
				X.ToString(format, formatProvider),
				Y.ToString(format, formatProvider)
			);
		}

	}

	[StructLayout(LayoutKind.Sequential)]
	public struct Vector2i
	{
		public int X;
		public int Y;

		public static Vector2i Zero => new Vector2i(0, 0);

		public Vector2i(int xy)
		{
			X = xy;
			Y = xy;
		}
		public Vector2i(int x, int y)
		{
			X = x;
			Y = y;
		}
		public Vector2i(Vector2i v)
		{
			X = v.X;
			Y = v.Y;
		}

		public static Vector2i operator +(Vector2i lhs, Vector2i rhs)
		{
			return new Vector2i(lhs.X + rhs.X, lhs.Y + rhs.Y);
		}
		public static Vector2i operator -(Vector2i lhs, Vector2i rhs)
		{
			return new Vector2i(lhs.X - rhs.X, lhs.Y - rhs.Y);
		}
		public static Vector2i operator *(Vector2i lhs, Vector2i rhs)
		{
			return new Vector2i(lhs.X * rhs.X, lhs.Y * rhs.Y);
		}
		public static Vector2i operator /(Vector2i lhs, Vector2i rhs)
		{
			return new Vector2i(lhs.X / rhs.X, lhs.Y / rhs.Y);
		}
		public static Vector2i operator *(Vector2i lhs, int s)
		{
			return new Vector2i(lhs.X * s, lhs.Y * s);
		}
		public static Vector2i operator /(Vector2i lhs, int s)
		{
			return new Vector2i(lhs.X / s, lhs.Y / s);
		}
		public static Vector2i operator *(int s, Vector2i rhs)
		{
			return new Vector2i(s * rhs.X, s * rhs.Y);
		}
		public static Vector2i operator /(int s, Vector2i rhs)
		{
			return new Vector2i(s / rhs.X, s / rhs.Y);
		}
		public static Vector2i operator +(Vector2i v)
		{
			return new Vector2i(v);
		}
		public static Vector2i operator -(Vector2i v)
		{
			return new Vector2i(-v.X, -v.Y);
		}

		public static explicit operator Vector2(Vector2i rhs)
		{
			return new Vector2(rhs.X, rhs.Y);
		}

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
			return string.Format("[{0}, {1}]",
				X.ToString(format, formatProvider),
				Y.ToString(format, formatProvider)
			);
		}
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct Vector2u
	{
		public uint X;
		public uint Y;

		public static Vector2u Zero => new Vector2u(0, 0);

		public Vector2u(uint xy)
		{
			X = xy;
			Y = xy;
		}
		public Vector2u(uint x, uint y)
		{
			X = x;
			Y = y;
		}
		public Vector2u(Vector2u v)
		{
			X = v.X;
			Y = v.Y;
		}

		public static Vector2u operator +(Vector2u lhs, Vector2u rhs)
		{
			return new Vector2u(lhs.X + rhs.X, lhs.Y + rhs.Y);
		}
		public static Vector2u operator -(Vector2u lhs, Vector2u rhs)
		{
			return new Vector2u(lhs.X - rhs.X, lhs.Y - rhs.Y);
		}
		public static Vector2u operator *(Vector2u lhs, Vector2u rhs)
		{
			return new Vector2u(lhs.X * rhs.X, lhs.Y * rhs.Y);
		}
		public static Vector2u operator /(Vector2u lhs, Vector2u rhs)
		{
			return new Vector2u(lhs.X / rhs.X, lhs.Y / rhs.Y);
		}
		public static Vector2u operator *(Vector2u lhs, uint s)
		{
			return new Vector2u(lhs.X * s, lhs.Y * s);
		}
		public static Vector2u operator /(Vector2u lhs, uint s)
		{
			return new Vector2u(lhs.X / s, lhs.Y / s);
		}
		public static Vector2u operator *(uint s, Vector2u rhs)
		{
			return new Vector2u(s * rhs.X, s * rhs.Y);
		}
		public static Vector2u operator /(uint s, Vector2u rhs)
		{
			return new Vector2u(s / rhs.X, s / rhs.Y);
		}
		
		public static explicit operator Vector2(Vector2u rhs)
		{
			return new Vector2(rhs.X, rhs.Y);
		}

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
			return string.Format("[{0}, {1}]",
				X.ToString(format, formatProvider),
				Y.ToString(format, formatProvider)
			);
		}
	}

}