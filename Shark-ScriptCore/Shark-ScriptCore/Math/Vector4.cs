
using System;
using System.Runtime.InteropServices;

namespace Shark
{

	[StructLayout(LayoutKind.Sequential)]
	public struct Vector4 : IFormattable
	{
		public float X;
		public float Y;
		public float Z;
		public float W;

		public static Vector4 Zero => new Vector4(0.0f);
		public static Vector4 One => new Vector4(1.0f);

		#region Swizzle
		public Vector2 XY { get => new Vector2(X, Y); set { X = value.X; Y = value.Y; } }
		public Vector2 XZ { get => new Vector2(X, Z); set { X = value.X; Z = value.Y; } }
		public Vector2 XW { get => new Vector2(X, W); set { X = value.X; W = value.Y; } }

		public Vector2 YX { get => new Vector2(Y, X); set { Y = value.X; X = value.Y; } }
		public Vector2 YZ { get => new Vector2(Y, Z); set { Y = value.X; Z = value.Y; } }
		public Vector2 YW { get => new Vector2(Y, W); set { Y = value.X; W = value.Y; } }

		public Vector2 ZX { get => new Vector2(Z, X); set { Z = value.X; X = value.Y; } }
		public Vector2 ZY { get => new Vector2(Z, Y); set { Z = value.X; Y = value.Y; } }
		public Vector2 ZW { get => new Vector2(Z, W); set { Z = value.X; W = value.Y; } }

		public Vector3 XYZ { get => new Vector3(X, Y, Z); set { X = value.X; Y = value.Y; Z = value.Z; } }
		public Vector3 XYW { get => new Vector3(X, Y, W); set { X = value.X; Y = value.Y; W = value.Z; } }
		public Vector3 XZY { get => new Vector3(X, Y, Z); set { X = value.X; Z = value.Y; Y = value.Z; } }
		public Vector3 XZW { get => new Vector3(X, Y, W); set { X = value.X; Z = value.Y; W = value.Z; } }

		public Vector3 YXZ { get => new Vector3(X, Y, Z); set { Y = value.X; X = value.Y; Z = value.Z; } }
		public Vector3 YXW { get => new Vector3(X, Y, W); set { Y = value.X; X = value.Y; W = value.Z; } }
		public Vector3 YZX { get => new Vector3(X, Y, Z); set { Y = value.X; Z = value.Y; X = value.Z; } }
		public Vector3 YZW { get => new Vector3(X, Y, W); set { Y = value.X; Z = value.Y; W = value.Z; } }

		public Vector3 ZXY { get => new Vector3(X, Y, Z); set { Z = value.X; X = value.Y; Y = value.Z; } }
		public Vector3 ZXW { get => new Vector3(X, Y, W); set { Z = value.X; X = value.Y; W = value.Z; } }
		public Vector3 ZYX { get => new Vector3(X, Y, Z); set { Z = value.X; Y = value.Y; X = value.Z; } }
		public Vector3 ZYW { get => new Vector3(X, Y, W); set { Z = value.X; Y = value.Y; W = value.Z; } }

		public Vector3 WXY { get => new Vector3(W, X, Y); set { W = value.X; X = value.Y; Y = value.Z; } }
		public Vector3 WXZ { get => new Vector3(W, X, Z); set { W = value.X; X = value.Y; Z = value.Z; } }
		public Vector3 WYX { get => new Vector3(W, Y, X); set { W = value.X; Y = value.Y; X = value.Z; } }
		public Vector3 WYZ { get => new Vector3(W, Y, Z); set { W = value.X; Y = value.Y; Z = value.Z; } }

		public Vector4 XYZW { get => new Vector4(X, Y, Z, W); set { X = value.X; Y = value.Y; Z = value.Z; W = value.W; } }
		public Vector4 XYWZ { get => new Vector4(X, Y, W, Z); set { X = value.X; Y = value.Y; W = value.Z; Z = value.W; } }
		public Vector4 XZYW { get => new Vector4(X, Z, Y, W); set { X = value.X; Z = value.Y; Y = value.Z; W = value.W; } }
		public Vector4 XZWY { get => new Vector4(X, Z, W, Y); set { X = value.X; Z = value.Y; W = value.Z; Y = value.W; } }
		public Vector4 XWYZ { get => new Vector4(X, W, Y, Z); set { X = value.X; W = value.Y; Y = value.Z; Z = value.W; } }
		public Vector4 XWZY { get => new Vector4(X, W, Z, Y); set { X = value.X; W = value.Y; Z = value.Z; Y = value.W; } }

		public Vector4 YXZW { get => new Vector4(Y, X, Z, W); set { Y = value.X; X = value.Y; Z = value.Z; W = value.W; } }
		public Vector4 YXWZ { get => new Vector4(Y, X, W, Z); set { Y = value.X; X = value.Y; W = value.Z; Z = value.W; } }
		public Vector4 YZXW { get => new Vector4(Y, Z, X, W); set { Y = value.X; Z = value.Y; Y = value.Z; W = value.W; } }
		public Vector4 YZWX { get => new Vector4(Y, Z, W, X); set { Y = value.X; Z = value.Y; W = value.Z; X = value.W; } }
		public Vector4 YWXZ { get => new Vector4(Y, W, X, Z); set { Y = value.X; W = value.Y; X = value.Z; Z = value.W; } }
		public Vector4 YWZX { get => new Vector4(Y, W, Z, X); set { Y = value.X; W = value.Y; Z = value.Z; X = value.W; } }

		public Vector4 ZXYW { get => new Vector4(Z, X, Y, W); set { Z = value.X; X = value.Y; Y = value.Z; W = value.W; } }
		public Vector4 ZXWY { get => new Vector4(Z, X, W, Y); set { Z = value.X; X = value.Y; W = value.Z; Y = value.W; } }
		public Vector4 ZYXW { get => new Vector4(Z, Y, X, W); set { Z = value.X; Y = value.Y; X = value.Z; W = value.W; } }
		public Vector4 ZYWX { get => new Vector4(Z, Y, W, X); set { Z = value.X; Y = value.Y; W = value.Z; X = value.W; } }
		public Vector4 ZWXY { get => new Vector4(Z, W, X, Y); set { Z = value.X; W = value.Y; X = value.Z; Y = value.W; } }
		public Vector4 ZWYX { get => new Vector4(Z, W, Y, X); set { Z = value.X; W = value.Y; Y = value.Z; X = value.W; } }

		public Vector4 WXYZ { get => new Vector4(W, X, Y, Z); set { W = value.X; X = value.Y; Y = value.Z; Z = value.W; } }
		public Vector4 WXZY { get => new Vector4(W, X, Z, Y); set { W = value.X; X = value.Y; Z = value.Z; Y = value.W; } }
		public Vector4 WYXZ { get => new Vector4(W, Y, X, Z); set { W = value.X; Y = value.Y; X = value.Z; Z = value.W; } }
		public Vector4 WYZX { get => new Vector4(W, Y, Z, X); set { W = value.X; Y = value.Y; Z = value.Z; X = value.W; } }
		public Vector4 WZXY { get => new Vector4(W, Z, X, Y); set { W = value.X; Z = value.Y; X = value.Z; Y = value.W; } }
		public Vector4 WZYX { get => new Vector4(W, Z, Y, X); set { W = value.X; Z = value.Y; Y = value.Z; X = value.W; } }
		#endregion

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

		public static Vector4 operator +(Vector4 lhs, Vector4 rhs) => new Vector4(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z, lhs.W + rhs.W);
		public static Vector4 operator -(Vector4 lhs, Vector4 rhs) => new Vector4(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z, lhs.W - rhs.W);
		public static Vector4 operator *(Vector4 lhs, Vector4 rhs) => new Vector4(lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z, lhs.W * rhs.W);
		public static Vector4 operator /(Vector4 lhs, Vector4 rhs) => new Vector4(lhs.X / rhs.X, lhs.Y / rhs.Y, lhs.Z / rhs.Z, lhs.W / rhs.W);
		public static Vector4 operator *(Vector4 lhs, float s)     => new Vector4(lhs.X * s, lhs.Y * s, lhs.Z * s, lhs.W * s);
		public static Vector4 operator /(Vector4 lhs, float s)     => new Vector4(lhs.X / s, lhs.Y / s, lhs.Z / s, lhs.W / s);
		public static Vector4 operator *(float s, Vector4 rhs)     => new Vector4(s * rhs.X, s * rhs.Y, s * rhs.Z, s * rhs.W);
		public static Vector4 operator /(float s, Vector4 rhs)     => new Vector4(s / rhs.X, s / rhs.Y, s / rhs.Z, s / rhs.W);
		public static Vector4 operator +(Vector4 v)                => new Vector4(v.X, v.Y, v.Z, v.W);
		public static Vector4 operator -(Vector4 v)                => new Vector4(-v.X, -v.Y, -v.Z, -v.W);

		public static bool operator ==(Vector4 lhs, Vector4 rhs) => lhs.X == rhs.X && lhs.Y == rhs.Y && lhs.Z == rhs.Z && lhs.W == rhs.W;
		public static bool operator !=(Vector4 lhs, Vector4 rhs) => !(lhs == rhs);

		public static Vector4 Lerp(Vector4 p0, Vector4 p1, float t)
		{
			return new Vector4(
				p0.X + (p1.X - p0.X) * t,
				p0.Y + (p1.Y - p0.Y) * t,
				p0.Z + (p1.Z - p0.Z) * t,
				p0.W + (p1.W - p0.W) * t
			);
		}
		public static float Dot(Vector4 a, Vector4 b)
		{
			Vector4 temp = a * b;
			return temp.X + temp.Y + temp.Z * temp.W;
		}
		public static Vector4 Sqrt(Vector4 v)
		{
			return new Vector4(
				Mathf.Sqrt(v.X),
				Mathf.Sqrt(v.Y),
				Mathf.Sqrt(v.Z),
				Mathf.Sqrt(v.W)
			);
		}
		public static Vector4 Normalize(Vector4 vec)
		{
			float length = vec.Length;
			if (length != 0)
				return vec / length;
			return Zero;
		}

		public float Length2
			=> Dot(this, this);

		public float Length
		{
			get => Mathf.Sqrt(Length2);
			set => this = Normalized * value;
		}

		public void Normalize()
			=> this = Normalize(this);

		public Vector4 Normalized
			=> Normalize(this);
		
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

		public override string ToString() => ToString(null, null);
		public string ToString(string format) => ToString(format, null);
		public string ToString(string format, IFormatProvider formatProvider)
		{
			return string.Format("[{0}, {1}, {2}, {3}]",
				X.ToString(format, formatProvider),
				Y.ToString(format, formatProvider),
				Z.ToString(format, formatProvider),
				W.ToString(format, formatProvider)
			);
		}

		public override bool Equals(object obj) => base.Equals(obj);
		public override int GetHashCode() => base.GetHashCode();
	}

}
