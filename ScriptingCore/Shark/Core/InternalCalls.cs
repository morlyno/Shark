using System.Runtime.CompilerServices;

namespace Shark
{
	internal static class InternalCalls
	{
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Log_LogLevel(Log.Level level, string msg);



		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Matrix4_Inverse(ref Matrix4 matrix, ref Matrix4 out_Result);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Matrix4 Matrix4_Mul(ref Matrix4 lhs, ref Matrix4 rhs);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector4 Matrix4_Mul(ref Matrix4 lhs, ref Vector4 rhs);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern UUID UUID_Generate();

	}

}
