
using System.Runtime.InteropServices;

namespace Shark
{

	[StructLayout(LayoutKind.Sequential)]
	public struct Transform
	{
		public Vector3 Translation;
		public Vector3 Rotation;
		public Vector3 Scale;
	}
	
	[StructLayout(LayoutKind.Sequential)]
	public struct Transform2D
	{
		public Vector2 Translation;
		public float Rotation;
		public Vector2 Scale;
	}
	
	[StructLayout(LayoutKind.Sequential)]
	public struct RigidBody2DTransform
	{
		public Vector2 Position;
		public float Rotation;
	}

}
