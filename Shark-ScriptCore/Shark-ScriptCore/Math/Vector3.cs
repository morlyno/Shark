
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

		public Vector2 XY => new Vector2(X, Y);
		public Vector2 XZ => new Vector2(X, Z);
		public Vector2 YZ => new Vector2(Y, Z);

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
			this.X = x;
			this.Y = y;
			this.Z = z;
		}
		public Vector3(Vector3 v)
		{
			X = v.X;
			Y = v.Y;
			Z = v.Z;
		}
		public Vector3(Vector2 xy, float z)
		{
			X = xy.X;
			Y = xy.Y;
			Z = z;
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


		public static Vector3 Lerp(Vector3 p0, Vector3 p1, float t)
		{
			return new Vector3(
				p0.X + (p1.X - p0.X) * t,
				p0.Y + (p1.Y - p0.Y) * t,
				p0.Z + (p1.Z - p0.Z) * t
			);
		}
		public static Vector3 Cross(Vector3 x, Vector3 y)
		{
			return new Vector3(
				x.Y * y.Z - y.Y * x.Z,
				x.Z * y.X - y.Z * x.X,
				x.X * y.Y - y.X * x.Y
			);
		}
		public static float Dot(Vector3 x, Vector3 y)
		{
			Vector3 temp = x * y;
			return temp.X + temp.Y + temp.Z;
		}
		public static Vector3 Sqrt(Vector3 v)
		{
			return new Vector3(
				(float)Math.Sqrt(v.X),
				(float)Math.Sqrt(v.Y),
				(float)Math.Sqrt(v.Z)
			);
		}
		public static Vector3 Normalize(Vector3 vec)
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
			return string.Format(
				"[{0}, {1}, {2}]",
				X.ToString(format, formatProvider),
				Y.ToString(format, formatProvider),
				Z.ToString(format, formatProvider)
			);
		}
	}

}