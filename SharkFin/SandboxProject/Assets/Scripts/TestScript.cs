using System;
using System.Runtime.InteropServices;

using Shark;

namespace Sandbox
{
	public class TestScript : Entity
	{
		private TimeStep m_Time = new TimeStep();

		public override void OnCreate()
		{
			Log.Info("TestScript::OnCreate");
		}

		public override void OnDestroy()
		{
			Log.Info("TestScript::OnDestroy");
		}

		public override void OnUpdate(TimeStep ts)
		{
			m_Time += ts;
			Log.Info("TestScript::OnUpdate {0}", m_Time);
		}

	}

	public static class Tests
	{
		public static void RunTest()
		{
			var mat4Size = Marshal.SizeOf(typeof(Matrix4));
			Log.Warn(mat4Size);
			//Log.Info(mat4Size);


			var s = new Matrix4(2.5f);
			var s2 = Matrix4.Inverse(s);

			Log.Info(s2);
			var v0 = Vector4.One;
			var v1 = s2 * v0;
			Log.Info(v1);

		}

	}

}
