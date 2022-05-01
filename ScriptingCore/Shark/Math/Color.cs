
using System;
using System.Runtime.InteropServices;

namespace Shark
{

	[StructLayout(LayoutKind.Sequential)]
	public struct Color : IFormattable
	{
		public float R;
		public float G;
		public float B;
		public float A;

		public static Color White   => new Color(1.0f, 1.0f, 1.0f, 1.0f);
		public static Color Back    => new Color(0.0f, 0.0f, 0.0f, 1.0f);
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
			R = r;
			G = g;
			B = b;
			A = 1.0f;
		}
		public Color(float r, float g, float b, float a)
		{
			R = r;
			G = g;
			B = b;
			A = a;
		}

		public Color(Vector4 v)
		{
			R = v.X;
			G = v.Y;
			B = v.Z;
			A = v.W;
		}

		public static Color operator+(Color lhs, Color rhs) { return new Color(lhs.R + rhs.R, lhs.G + rhs.G, lhs.B + rhs.B, lhs.A + rhs.A); }
		public static Color operator-(Color lhs, Color rhs) { return new Color(lhs.R - rhs.R, lhs.G - rhs.G, lhs.B - rhs.B, lhs.A - rhs.A); }
		public static Color operator*(Color lhs, Color rhs) { return new Color(lhs.R * rhs.R, lhs.G * rhs.G, lhs.B * rhs.B, lhs.A * rhs.A); }
		public static Color operator*(Color lhs, float scale) { return new Color(lhs.R * scale, lhs.G * scale, lhs.B * scale, lhs.A * scale); }
		public static Color operator*(float scale, Color rhs) { return new Color(scale * rhs.R, scale * rhs.G, scale * rhs.B, scale * rhs.A); }
		public static Color operator/(Color lhs, float scale) { return new Color(lhs.R / scale, lhs.G / scale, lhs.B / scale, lhs.A / scale); }

		public float this[int index]
		{
			get
			{
				switch (index)
				{
					case 0: return R;
					case 1: return G;
					case 2: return B;
					case 3: return A;
					default:
						throw new System.IndexOutOfRangeException($"Index between 0 and 3 expected but {index} provided");
				}
			}
			set
			{
				switch (index)
				{
					case 0: R = value; break;
					case 1: G = value; break;
					case 2: B = value; break;
					case 3: A = value; break;
					default:
						throw new System.IndexOutOfRangeException($"Index between 0 and 3 expected but {index} provided");
				}
			}
		}

		public static explicit operator Color(Vector4 v) { return new Color(v.X, v.Y, v.Z, v.W);}
		public static explicit operator Vector4(Color c) { return new Vector4(c.R, c.G, c.B, c.A); }

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
				R.ToString(format, formatProvider),
				G.ToString(format, formatProvider),
				B.ToString(format, formatProvider),
				A.ToString(format, formatProvider)
			);
		}
	}

}
