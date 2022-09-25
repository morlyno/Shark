
using System;
using System.Runtime.InteropServices;

namespace Shark
{

	[StructLayout(LayoutKind.Sequential)]
	public struct Color : IFormattable
	{
		public float r;
		public float g;
		public float b;
		public float a;

		public static Color White   => new Color(1.0f, 1.0f, 1.0f, 1.0f);
		public static Color Black    => new Color(0.0f, 0.0f, 0.0f, 1.0f);
		public static Color Red     => new Color(1.0f, 0.0f, 0.0f, 1.0f);
		public static Color Green   => new Color(0.0f, 1.0f, 0.0f, 1.0f);
		public static Color Blue    => new Color(0.0f, 0.0f, 1.0f, 1.0f);
		public static Color Yellow  => new Color(1.0f, 1.0f, 0.0f, 1.0f);
		public static Color Cyan    => new Color(0.0f, 1.0f, 1.0f, 1.0f);
		public static Color Magenta => new Color(1.0f, 0.0f, 1.0f, 1.0f);
		public static Color Gray    => new Color(0.5f, 0.5f, 0.5f, 1.0f);
		public static Color Clear   => new Color(0.0f, 0.0f, 0.0f, 0.0f);

		public Color(float r, float g, float b)
		{
			this.r = r;
			this.g = g;
			this.b = b;
			this.a = 1.0f;
		}
		public Color(float r, float g, float b, float a)
		{
			this.r = r;
			this.g = g;
			this.b = b;
			this.a = a;
		}
		public Color(Vector4 v)
		{
			this.r = v.X;
			this.g = v.Y;
			this.b = v.Z;
			this.a = v.W;
		}

		public static Color operator+(Color lhs, Color rhs) { return new Color(lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b, lhs.a + rhs.a); }
		public static Color operator-(Color lhs, Color rhs) { return new Color(lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b, lhs.a - rhs.a); }
		public static Color operator*(Color lhs, Color rhs) { return new Color(lhs.r * rhs.r, lhs.g * rhs.g, lhs.b * rhs.b, lhs.a * rhs.a); }
		public static Color operator*(Color lhs, float scale) { return new Color(lhs.r * scale, lhs.g * scale, lhs.b * scale, lhs.a * scale); }
		public static Color operator*(float scale, Color rhs) { return new Color(scale * rhs.r, scale * rhs.g, scale * rhs.b, scale * rhs.a); }
		public static Color operator/(Color lhs, float scale) { return new Color(lhs.r / scale, lhs.g / scale, lhs.b / scale, lhs.a / scale); }

		public float this[int index]
		{
			get
			{
				switch (index)
				{
					case 0: return r;
					case 1: return g;
					case 2: return b;
					case 3: return a;
					default:
						throw new System.IndexOutOfRangeException($"Index between 0 and 3 expected but {index} provided");
				}
			}
			set
			{
				switch (index)
				{
					case 0: r = value; break;
					case 1: g = value; break;
					case 2: b = value; break;
					case 3: a = value; break;
					default:
						throw new System.IndexOutOfRangeException($"Index between 0 and 3 expected but {index} provided");
				}
			}
		}

		public static explicit operator Color(Vector4 v) { return new Color(v.X, v.Y, v.Z, v.W);}
		public static explicit operator Vector4(Color c) { return new Vector4(c.r, c.g, c.b, c.a); }

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
				"RGBA [{0}, {1}, {2}, {3}]",
				r.ToString(format, formatProvider),
				g.ToString(format, formatProvider),
				b.ToString(format, formatProvider),
				a.ToString(format, formatProvider)
			);
		}
	}

}
