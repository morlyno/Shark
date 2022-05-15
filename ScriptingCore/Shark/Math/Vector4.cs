
using System;
using System.Runtime.InteropServices;

namespace Shark
{

	[StructLayout(LayoutKind.Sequential)]
	public struct Vector4 : IFormattable
	{
		public float x;
		public float y;
		public float z;
		public float w;

		public static Vector4 Zero => new Vector4(0.0f);
		public static Vector4 One => new Vector4(1.0f);

		public Vector4(float xyzw)
		{
			x = xyzw;
			y = xyzw;
			z = xyzw;
			w = xyzw;
		}
		public Vector4(float x, float y, float z, float w)
		{
			this.x = x;
			this.y = y;
			this.z = z;
			this.w = w;
		}
		public Vector4(Vector4 vec)
		{
			x = vec.x;
			y = vec.y;
			z = vec.z;
			w = vec.w;
		}

		public static Vector4 operator +(Vector4 lhs, Vector4 rhs)
		{
			return new Vector4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
		}
		public static Vector4 operator -(Vector4 lhs, Vector4 rhs)
		{
			return new Vector4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
		}
		public static Vector4 operator *(Vector4 lhs, Vector4 rhs)
		{
			return new Vector4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
		}
		public static Vector4 operator /(Vector4 lhs, Vector4 rhs)
		{
			return new Vector4(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w);
		}
		public static Vector4 operator *(Vector4 lhs, float s)
		{
			return new Vector4(lhs.x * s, lhs.y * s, lhs.z * s, lhs.w * s);
		}
		public static Vector4 operator /(Vector4 lhs, float s)
		{
			return new Vector4(lhs.x / s, lhs.y / s, lhs.z / s, lhs.w / s);
		}
		public static Vector4 operator *(float s, Vector4 rhs)
		{
			return new Vector4(s * rhs.x, s * rhs.y, s * rhs.z, s * rhs.w);
		}
		public static Vector4 operator /(float s, Vector4 rhs)
		{
			return new Vector4(s / rhs.x, s / rhs.y, s / rhs.z, s / rhs.w);
		}
		public static Vector4 operator +(Vector4 v)
		{
			return new Vector4(v.x, v.y, v.z, v.w);
		}
		public static Vector4 operator -(Vector4 v)
		{
			return new Vector4(-v.x, -v.y, -v.z, -v.w);
		}


		public static Vector4 Lerp(Vector4 p0, Vector4 p1, float t)
		{
			return new Vector4(
				p0.x + (p1.x - p0.x) * t,
				p0.y + (p1.y - p0.y) * t,
				p0.z + (p1.z - p0.z) * t,
				p0.w + (p1.w - p0.w) * t
			);
		}
		public static float Dot(Vector4 a, Vector4 b)
		{
			Vector4 temp = a * b;
			return temp.x + temp.y + temp.z * temp.w;
		}
		public static Vector4 Sqrt(Vector4 v)
		{
			return new Vector4(
				Mathf.Sqrt(v.x),
				Mathf.Sqrt(v.y),
				Mathf.Sqrt(v.z),
				Mathf.Sqrt(v.w)
			);
		}
		public static Vector4 Normalize(Vector4 vec)
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
			get
			{
				return Mathf.Sqrt(x * y * z * w);
			}
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
				switch (index)
				{
					case 0: return x;
					case 1: return y;
					case 2: return z;
					case 3: return w;
					default:
						throw new System.IndexOutOfRangeException($"Index between 0 and 3 expected but {index} provided");
				}
			}
			set
			{
				switch (index)
				{
					case 0: x = value; break;
					case 1: y = value; break;
					case 2: z = value; break;
					case 3: w = value; break;
					default:
						throw new System.IndexOutOfRangeException($"Index between 0 and 3 expected but {index} provided");
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
			return string.Format("[{0}, {1}, {2}, {3}]",
				x.ToString(format, formatProvider),
				y.ToString(format, formatProvider),
				z.ToString(format, formatProvider),
				w.ToString(format, formatProvider)
			);
		}
	}

}
