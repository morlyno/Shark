
using System;

namespace Shark
{

	public struct Vector4 : IFormattable
	{
		public float X;
		public float Y;
		public float Z;
		public float W;

		public static Vector4 Zero => new Vector4(0.0f);
		public static Vector4 One => new Vector4(1.0f);

		public Vector4(float xyzw)
		{
			X = xyzw;
			Y = xyzw;
			Z = xyzw;
			W = xyzw;
		}
		public Vector4(float x, float y, float z, float w)
		{
			X = x;
			Y = y;
			Z = z;
			W = w;
		}
		public Vector4(Vector4 vec)
		{
			X = vec.X;
			Y = vec.Y;
			Z = vec.Z;
			W = vec.W;
		}

		public static Vector4 operator +(Vector4 lhs, Vector4 rhs)
		{
			return new Vector4(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z, lhs.W + rhs.W);
		}
		public static Vector4 operator -(Vector4 lhs, Vector4 rhs)
		{
			return new Vector4(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z, lhs.W - rhs.W);
		}
		public static Vector4 operator *(Vector4 lhs, Vector4 rhs)
		{
			return new Vector4(lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z, lhs.W * rhs.W);
		}
		public static Vector4 operator /(Vector4 lhs, Vector4 rhs)
		{
			return new Vector4(lhs.X / rhs.X, lhs.Y / rhs.Y, lhs.Z / rhs.Z, lhs.W / rhs.W);
		}
		public static Vector4 operator *(Vector4 lhs, float s)
		{
			return new Vector4(lhs.X * s, lhs.Y * s, lhs.Z * s, lhs.W * s);
		}
		public static Vector4 operator /(Vector4 lhs, float s)
		{
			return new Vector4(lhs.X / s, lhs.Y / s, lhs.Z / s, lhs.W / s);
		}
		public static Vector4 operator *(float s, Vector4 rhs)
		{
			return new Vector4(s * rhs.X, s * rhs.Y, s * rhs.Z, s * rhs.W);
		}
		public static Vector4 operator /(float s, Vector4 rhs)
		{
			return new Vector4(s / rhs.X, s / rhs.Y, s / rhs.Z, s / rhs.W);
		}
		public static Vector4 operator +(Vector4 v)
		{
			return new Vector4(v.X, v.Y, v.Z, v.W);
		}
		public static Vector4 operator -(Vector4 v)
		{
			return new Vector4(-v.X, -v.Y, -v.Z, -v.W);
		}

		public float Dot(Vector4 other)
		{
			return Mathf.Dot(this, other);
		}

		public float Length()
		{
			return Mathf.Sqrt(X * Y * Z * W);
		}

		public Vector4 Normalize()
		{
			float length = Length();
			if (length == 0)
			{
				return new Vector4(0.0f);
			}

			return this / length;
		}

		public float this[int index]
		{
			get
			{
				switch (index)
				{
					case 0: return X;
					case 1: return Y;
					case 2: return Z;
					case 3: return W;
					default:
						throw new System.IndexOutOfRangeException($"Index between 0 and 3 expected but {index} provided");
				}
			}
			set
			{
				switch (index)
				{
					case 0: X = value; break;
					case 1: Y = value; break;
					case 2: Z = value; break;
					case 3: W = value; break;
					default:
						throw new System.IndexOutOfRangeException($"Index between 0 and 3 expected but {index} provided");
				}
			}
		}


		public string ToString(string format, IFormatProvider formatProvider)
		{
			return string.Format("[{0}, {1}, {2}, {3}]",
				X.ToString(format, formatProvider),
				Y.ToString(format, formatProvider),
				Z.ToString(format, formatProvider),
				W.ToString(format, formatProvider)
			);
		}
	}

}
