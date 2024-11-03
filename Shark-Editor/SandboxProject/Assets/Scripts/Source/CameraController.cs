
using Shark;

namespace Sandbox
{
	public class CameraController : Entity
	{
		public float MovementSpeed = 1.0f;
		public float ZoomSpeed = 1.0f;

		protected override void OnUpdate(float ts)
		{
			Vector3 direction = Vector3.Zero;

			if (Input.IsKeyDown(KeyCode.A))
				direction += Vector3.Left;

			if (Input.IsKeyDown(KeyCode.D))
				direction += Vector3.Right;

			if (Input.IsKeyDown(KeyCode.W))
				direction += Vector3.Up;

			if (Input.IsKeyDown(KeyCode.S))
				direction += Vector3.Down;

			Translation += direction * ts * MovementSpeed;

			if (Input.MouseScroll != 0)
			{
				Translation += new Vector3(0.0f, 0.0f, Input.MouseScroll * ZoomSpeed);
			}
		}
	}
}
