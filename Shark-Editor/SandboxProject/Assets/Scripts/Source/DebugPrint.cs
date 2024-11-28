
using Shark;

namespace Sandbox
{
	public class DebugPrint : Entity
	{
		public string Message = "Test Message";

		protected override void OnCreate()
		{
			Log.Critical(Message);
		}

	}
}
