
using System;
using System.Runtime.InteropServices;

namespace Shark
{

	[StructLayout(LayoutKind.Sequential)]
	public struct Vector3 : IFormattable
	{
		public float x;
		public float y;
		public float z;

		public Vector2 XY => new Vector2(x, y);
		public Vector2 XZ => new Vector2(x, z);
		public Vector2 YZ => new Vector2(y, z);

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
			x = xyz;
			y = xyz;
			z = xyz;
		}
		public Vector3(float x, float y, float z)
		{
			this.x = x;
			this.y = y;
			this.z = z;
		}
		public Vector3(Vector3 v)
		{
			x = v.x;
			y = v.y;
			z = v.z;
		}

		public static Vector3 operator +(Vector3 lhs, Vector3 rhs)
		{
			return new Vector3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
		}
		public static Vector3 operator -(Vector3 lhs, Vector3 rhs)
		{
			return new Vector3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
		}
		public static Vector3 operator *(Vector3 lhs, Vector3 rhs)
		{
			return new Vector3(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
		}
		public static Vector3 operator /(Vector3 lhs, Vector3 rhs)
		{
			return new Vector3(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z);
		}
		public static Vector3 operator *(Vector3 lhs, float s)
		{
			return new Vector3(lhs.x * s, lhs.y * s, lhs.z * s);
		}
		public static Vector3 operator /(Vector3 lhs, float s)
		{
			return new Vector3(lhs.x / s, lhs.y / s, lhs.z / s);
		}
		public static Vector3 operator *(float s, Vector3 rhs)
		{
			return new Vector3(s * rhs.x, s * rhs.y, s * rhs.z);
		}
		public static Vector3 operator /(float s, Vector3 rhs)
		{
			return new Vector3(s / rhs.x, s / rhs.y, s / rhs.z);
		}
		public static Vector3 operator +(Vector3 v)
		{
			return new Vector3(v);
		}
		public static Vector3 operator -(Vector3 v)
		{
			return new Vector3(-v.x, -v.y, -v.z);
		}


		public static Vector3 Lerp(Vector3 p0, Vector3 p1, float t)
		{
			return new Vector3(
				p0.x + (p1.x - p0.x) * t,
				p0.y + (p1.y - p0.y) * t,
				p0.z + (p1.z - p0.z) * t
			);
		}
		public static Vector3 Cross(Vector3 x, Vector3 y)
		{
			return new Vector3(
				x.y * y.z - y.y * x.z,
				x.z * y.x - y.z * x.x,
				x.x * y.y - y.x * x.y
			);
		}
		public static float Dot(Vector3 x, Vector3 y)
		{
			Vector3 temp = x * y;
			return temp.x + temp.y + temp.z;
		}
		public static Vector3 Sqrt(Vector3 v)
		{
			return new Vector3(
				(float)Math.Sqrt(v.x),
				(float)Math.Sqrt(v.y),
				(float)Math.Sqrt(v.z)
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
					return x;
				}
				else if (index == 1)
				{
					return y;
				}
				else if (index == 2)
				{
					return z;
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
					x = value;
				}
				else if (index == 1)
				{
					y = value;
				}
				else if (index == 2)
				{
					z = value;
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
				x.ToString(format, formatProvider),
				y.ToString(format, formatProvider),
				z.ToString(format, formatProvider)
			);
		}
	}

}