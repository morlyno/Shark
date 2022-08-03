
using Shark;

namespace Sandbox
{
	public class CameraScript : Entity
	{
		public float MovementSpeed = 10.0f;

		protected override void OnCreate()
		{
		}

		protected override void OnDestroy()
		{
		}

		protected override void OnUpdate(float ts)
		{
			Move(ts);
		}


		private void Move(float ts)
		{
			var translation = Transform.Translation;

			float delta = MovementSpeed * ts;

			if (Input.IsKeyPressed(Key.W))
				translation.y += delta;
			if (Input.IsKeyPressed(Key.A))
				translation.x -= delta;
			if (Input.IsKeyPressed(Key.S))
				translation.y -= delta;
			if (Input.IsKeyPressed(Key.D))
				translation.x += delta;

			Transform.Translation = translation;
		}

	}
}
