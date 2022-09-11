using Shark;

namespace Sandbox
{
	public class ChildController : Entity
	{
		private float m_MovementSpeed = 5.0f;
		private bool m_LocalMode = true;

		protected override void OnCreate()
		{
		}

		protected override void OnUpdate(float ts)
		{
			Vector3 delta = Vector3.Zero;
			if (Input.IsKeyPressed(Key.LeftArrow))
				delta += Vector3.Left;
			
			if (Input.IsKeyPressed(Key.RightArrow))
				delta += Vector3.Right;
			
			if (Input.IsKeyPressed(Key.UpArrow))
				delta += Vector3.Up;

			if (Input.IsKeyPressed(Key.DownArrow))
				delta += Vector3.Down;

			if (m_LocalMode)
			{
				var transform = Transform.LocalTransform;
				transform.Translation += delta * m_MovementSpeed * ts;
				Transform.LocalTransform = transform;
			}
			else
			{
				var transform = Transform.WorldTransform;
				transform.Translation += delta * m_MovementSpeed * ts;
				Transform.WorldTransform = transform;
			}
		}

		protected override void OnUIRender()
		{
			EditorUI.BeginWindow("C# Test");
			var local = Transform.LocalTransform;
			EditorUI.Text("Local Transform");
			EditorUI.Text("Translation: {0}", local.Translation);
			EditorUI.Text("Rotation:    {0}", local.Rotation);
			EditorUI.Text("Scale:       {0}", local.Scale);

			//UI.NewLine();

			var world = Transform.WorldTransform;
			EditorUI.Text("World Transform");
			EditorUI.Text("Translation: {0}", world.Translation);
			EditorUI.Text("Rotation:    {0}", world.Rotation);
			EditorUI.Text("Scale:       {0}", world.Scale);
			EditorUI.EndWindow();
		}

	}
}
