
using Shark;
using System;

namespace Sandbox
{
	public class CameraScript : Entity
	{
		public float MovementSpeed = 10.0f;
		private Random m_Rng = new Random();


		void OnCreate()
		{
		}

		void OnDestroy()
		{
		}

		void OnUpdate(TimeStep ts)
		{
			Move(ts);
			//TestEvents();

		}

		private void CreateEntities()
		{
			for (uint i = 0; i < 200; i++)
			{
				var testScript = Scene.Instantiate<TestScript>("TestScript");
				//testScript.SetOffset(new Vector3(
				//	(float)m_Rng.NextDouble() * 20.0f - 10.0f,
				//	(float)m_Rng.NextDouble() * 20.0f - 10.0f,
				//	0.0f
				//));
			}
		}

		private void Move(TimeStep ts)
		{
			var translation = Transform.Translation;

			if (Input.KeyPressed(Key.W))
				translation.y += MovementSpeed * ts;
			if (Input.KeyPressed(Key.A))
				translation.x -= MovementSpeed * ts;
			if (Input.KeyPressed(Key.S))
				translation.y -= MovementSpeed * ts;
			if (Input.KeyPressed(Key.D))
				translation.x += MovementSpeed * ts;

			Transform.Translation = translation;
		}

	}
}
