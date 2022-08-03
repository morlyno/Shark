
using Shark;

namespace Sandbox
{
	public class BasicScript : Entity
	{
		protected override void OnCreate()
		{
			Log.Info("OnCreate");

			Entity e = Scene.GetEntityByTag("Camera");
			Log.Info(e);
			var cam = e.GetComponent<CameraComponent>();
			Log.Info(cam.Type);
		}

		protected override void OnCollishionBegin(Collider2D collider)
		{
			Log.Info("OnCollishionBegin");
		}
	}
}
