﻿
using System;
using System.Runtime.InteropServices;

namespace Shark
{

	[StructLayout(LayoutKind.Sequential)]
	public struct Matrix4
	{
		public Vector4 Collumn0;
		public Vector4 Collumn1;
		public Vector4 Collumn2;
		public Vector4 Collumn3;

		public static Matrix4 Identity
		{
			get => new Matrix4(
				new Vector4(1.0f, 0.0f, 0.0f, 0.0f),
				new Vector4(0.0f, 1.0f, 0.0f, 0.0f),
				new Vector4(0.0f, 0.0f, 1.0f, 0.0f),
				new Vector4(0.0f, 0.0f, 0.0f, 1.0f)
			);
		}

		public Matrix4(float val = 0.0f)
		{
			Collumn0 = new Vector4(val, 0.0f, 0.0f, 0.0f);
			Collumn1 = new Vector4(0.0f, val, 0.0f, 0.0f);
			Collumn2 = new Vector4(0.0f, 0.0f, val, 0.0f);
			Collumn3 = new Vector4(0.0f, 0.0f, 0.0f, val);
		}
		public Matrix4(Vector4 c0, Vector4 c1, Vector4 c2, Vector4 c3)
		{
			Collumn0 = c0;
			Collumn1 = c1;
			Collumn2 = c2;
			Collumn3 = c3;
		}

		public static Matrix4 operator*(Matrix4 left, Matrix4 right)
		{
			Matrix4 result = new();
			unsafe { InternalCalls.Matrix4_Matrix4MulMatrix4(&left, &right, &result); }
			return result;
		}
		public static Vector4 operator*(Matrix4 left, Vector4 right)
		{
			Vector4 result = new();
			unsafe { InternalCalls.Matrix4_Matrix4MulVector4(&left, &right, &result); }
			return result;
		}


		public static Matrix4 Inverse(Matrix4 mat4)
		{
			Matrix4 result = new();
			unsafe { InternalCalls.Matrix4_Inverse(&mat4, &result); }
			return result;
		}
		public Matrix4 Inverse()
		{
			return Inverse(this);
		}


		public Vector4 this[int index]
		{
			get
			{
				switch (index)
				{
					case 0: return Collumn0;
					case 1: return Collumn1;
					case 2: return Collumn2;
					case 3: return Collumn3;
					default:
						throw new System.IndexOutOfRangeException($"Index between 0 and 3 expected but {index} provided");
				}
			}
			set
			{
				switch (index)
				{
					case 0: Collumn0 = value; break;
					case 1: Collumn1 = value; break;
					case 2: Collumn2 = value; break;
					case 3: Collumn3 = value; break;
					default:
						throw new System.IndexOutOfRangeException($"Index between 0 and 3 expected but {index} provided");
				}
			}
		}
		public float this[int col, int row]
		{
			get => this[col][row];
			set
			{
				Vector4 vec = this[col];
				vec[row] = value;
				this[col] = vec;
			}
		}

	}

}
