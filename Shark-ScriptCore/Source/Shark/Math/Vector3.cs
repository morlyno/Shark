﻿
using System;
using System.Runtime.InteropServices;

namespace Shark
{

	[StructLayout(LayoutKind.Sequential)]
	public struct Vector3
	{
		public float X;
		public float Y;
		public float Z;

		#region Swizzle
		public Vector2 XY { get => new Vector2(X, Y); set { X = value.X; Y = value.Y; } }
		public Vector2 XZ { get => new Vector2(X, Z); set { X = value.X; Z = value.Y; } }
		public Vector2 YX { get => new Vector2(Y, X); set { Y = value.X; X = value.Y; } }
		public Vector2 YZ { get => new Vector2(Y, Z); set { Y = value.X; Z = value.Y; } }
		public Vector2 ZX { get => new Vector2(Z, X); set { Z = value.X; X = value.Y; } }
		public Vector2 ZY { get => new Vector2(Z, Y); set { Z = value.X; Y = value.Y; } }

		public Vector3 XYZ { get => new Vector3(X, Y, Z); set { X = value.X; Y = value.Y; Z = value.Z; } }
		public Vector3 XZY { get => new Vector3(X, Y, Z); set { X = value.X; Z = value.Y; Y = value.Z; } }
		public Vector3 YXZ { get => new Vector3(X, Y, Z); set { Y = value.X; X = value.Y; Z = value.Z; } }
		public Vector3 YZX { get => new Vector3(X, Y, Z); set { Y = value.X; Z = value.Y; X = value.Z; } }
		public Vector3 ZXY { get => new Vector3(X, Y, Z); set { Z = value.X; X = value.Y; Y = value.Z; } }
		public Vector3 ZYX { get => new Vector3(X, Y, Z); set { Z = value.X; Y = value.Y; X = value.Z; } }
		#endregion

		public static Vector3 Zero      => new Vector3(0.0f);
		public static Vector3 One       => new Vector3(1.0f);

		public static Vector3 Right     => new Vector3( 1.0f,  0.0f,  0.0f);
		public static Vector3 Left      => new Vector3(-1.0f,  0.0f,  0.0f);
		public static Vector3 Up        => new Vector3( 0.0f,  1.0f,  0.0f);
		public static Vector3 Down      => new Vector3( 0.0f, -1.0f,  0.0f);
		public static Vector3 Forwards  => new Vector3( 0.0f,  0.0f,  1.0f);
		public static Vector3 Backwards => new Vector3( 0.0f,  0.0f, -1.0f);

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
		public Vector3(Vector2 xy, float z = 0.0f)
		{
			X = xy.X;
			Y = xy.Y;
			Z = z;
		}
		public Vector3(float x, Vector2 yz)
		{
			X = x;
			Y = yz.X;
			Z = yz.Y;
		}

		public static Vector3 operator +(Vector3 lhs, Vector3 rhs) => new Vector3(lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z);
		public static Vector3 operator -(Vector3 lhs, Vector3 rhs) => new Vector3(lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z);
		public static Vector3 operator *(Vector3 lhs, Vector3 rhs) => new Vector3(lhs.X * rhs.X, lhs.Y * rhs.Y, lhs.Z * rhs.Z);
		public static Vector3 operator /(Vector3 lhs, Vector3 rhs) => new Vector3(lhs.X / rhs.X, lhs.Y / rhs.Y, lhs.Z / rhs.Z);
		public static Vector3 operator *(Vector3 lhs, float s)     => new Vector3(lhs.X * s, lhs.Y * s, lhs.Z * s);
		public static Vector3 operator /(Vector3 lhs, float s)     => new Vector3(lhs.X / s, lhs.Y / s, lhs.Z / s);
		public static Vector3 operator *(float s, Vector3 rhs)     => new Vector3(s * rhs.X, s * rhs.Y, s * rhs.Z);
		public static Vector3 operator /(float s, Vector3 rhs)     => new Vector3(s / rhs.X, s / rhs.Y, s / rhs.Z);
		public static Vector3 operator +(Vector3 v)                => new Vector3(v.X, v.Y, v.Z);
		public static Vector3 operator -(Vector3 v)                => new Vector3(-v.X, -v.Y, -v.Z);

		public override bool Equals(object? obj) => obj is Vector3 other && Equals(other);
		public bool Equals(Vector3 right) => X == right.X && Y == right.Y && Z == right.Z;

		public static bool operator ==(Vector3 left, Vector3 right) => left.Equals(right);
		public static bool operator !=(Vector3 left, Vector3 right) => !left.Equals(right);

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
				Mathf.Sqrt(v.X),
				Mathf.Sqrt(v.Y),
				Mathf.Sqrt(v.Z)
			);
		}
		public static Vector3 Normalize(Vector3 vec)
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

		public Vector3 Normalized
			=> Normalize(this);

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

		public override string ToString() => $"Vector3[{X}, {Y}, {Z}]";
		public override int GetHashCode() => (X, Y, Z).GetHashCode();
	}

}