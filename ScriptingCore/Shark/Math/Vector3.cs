
using System;
using System.Runtime.InteropServices;

namespace Shark
{

	[StructLayout(LayoutKind.Sequential)]
	public struct Vector3 : IFormattable
	{
		public float X;
		public float Y;
		public float Z;

		public static Vector3 Zero => new Vector3(0.0f);
		public static Vector3 One => new Vector3(1.0f);

		public static Vector3 Right => new Vector3(1.0f, 0.0f, 0.0f);
		public static Vector3 Left => new Vector3(-1.0f, 0.0f, 0.0f);
		public static Vector3 Up => new Vector3(0.0f, 1.0f, 0.0f);
		public static Vector3 Down => new Vector3(0.0f, -1.0f, 0.0f);
		public static Vector3 Forwards => new Vector3(0.0f, 0.0f, 1.0f);
		public static Vector3 Backwards => new Vector3(0.0f, 0.0f, -1.0f);

		public Vector3(float xyz)
		{
			X = xyz;
			Y = xyz;
			Z = xyz;
		}
		public Vector3(float x, float y, float z)
		{
			X = x;
			Y = y;
			Z = z;
		}
		public Vector3(Vector3 v)
		{
			X = v.X;
			Y = v.Y;
			Z = v.Z;
		}

		public static Vector3 operator +(Vector3 lhs, Vector3 rhs)
		{
			return new Vector3(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z);
		}
		public static Vector3 operator -(Vector3 lhs, Vector3 rhs)
		{
			return new Vector3(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z);
		}
		public static Vector3 operator *(Vector3 lhs, Vector3 rhs)
		{
			return new Vector3(lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z);
		}
		public static Vector3 operator /(Vector3 lhs, Vector3 rhs)
		{
			return new Vector3(lhs.X / rhs.X, lhs.Y / rhs.Y, lhs.Z / rhs.Z);
		}
		public static Vector3 operator *(Vector3 lhs, float s)
		{
			return new Vector3(lhs.X * s, lhs.Y * s, lhs.Z * s);
		}
		public static Vector3 operator /(Vector3 lhs, float s)
		{
			return new Vector3(lhs.X / s, lhs.Y / s, lhs.Z / s);
		}
		public static Vector3 operator *(float s, Vector3 rhs)
		{
			return new Vector3(s * rhs.X, s * rhs.Y, s * rhs.Z);
		}
		public static Vector3 operator /(float s, Vector3 rhs)
		{
			return new Vector3(s / rhs.X, s / rhs.Y, s / rhs.Z);
		}
		public static Vector3 operator +(Vector3 v)
		{
			return new Vector3(v);
		}
		public static Vector3 operator -(Vector3 v)
		{
			return new Vector3(-v.X, -v.Y, -v.Z);
		}

		public Vector3 Cross(Vector3 other)
		{
			return Mathf.Cross(this, other);
		}

		public float Dot(Vector3 other)
		{
			return Mathf.Dot(this, other);
		}

		public float Length()
		{
			return Mathf.Sqrt(X * Y * Z);
		}

		public Vector3 Normalize()
		{
			float length = Length();
			if (length == 0)
			{
				return new Vector3(0.0f);
			}

			return this / length;
		}

		public static Vector3 Lerp(Vector3 p0, Vector3 p1, float t)
		{
			return new Vector3(
				p0.X + (p1.X - p0.X) * t,
				p0.Y + (p1.Y - p0.Y) * t,
				p0.Z + (p1.Z - p0.Z) * t
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
				else if (index == 2)
				{
					return Z;
				}
				else
				{
					throw new System.IndexOutOfRangeException($"Index between 0 and 2 expected but {index} provided");
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
				else if (index == 2)
				{
					Z = value;
				}
				else
				{
					throw new System.IndexOutOfRangeException($"Index between 0 and 2 expected but {index} provided");
				}
			}
		}

		public string ToString(string format, IFormatProvider formatProvider)
		{
			return string.Format("[{0}, {1}, {2}]", X, Y, Z);
		}
	}

}