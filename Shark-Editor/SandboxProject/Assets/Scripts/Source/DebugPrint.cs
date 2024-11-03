
using Shark;

namespace Sandbox
{
	public class DebugPrint : Entity
	{
		public string Message;

		protected override void OnCreate()
		{
			Log.Critical(Message);
		}

	}
}
