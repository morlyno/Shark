
using Shark;
using Shark.MouseEvents;

namespace Sandbox
{
	public class CameraScript : Entity
	{
		public float MovementSpeed = 10.0f;

		protected override void OnCreate()
		{
			EventHandler.OnMouseScrolled += OnMouseScrolled;
		}

		protected override void OnDestroy()
		{
			EventHandler.OnMouseScrolled -= OnMouseScrolled;
		}

		protected override void OnUpdate(TimeStep ts)
		{
			Move(ts);
		}

		void OnMouseScrolled(MouseScrolledEvent e)
		{
			var translation = Transform.Translation;
			translation.z += e.Delta;
			Transform.Translation = translation;
		}

		private void Move(TimeStep ts)
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
