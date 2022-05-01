
using Shark;

namespace Sandbox
{

	public class PlayerController : Entity
	{
		private bool m_KeyControlSpace = false;

		// Movement
		private float m_MovementSpeed = 5.0f;

		private Vector2 m_JumpForce = Vector2.Up * 15000.0f;

		private RigidBody2DComponent m_RigidBody;

		void OnCreate()
		{
			if (!HasComponent<RigidBody2DComponent>())
				AddComponent<RigidBody2DComponent>();

			m_RigidBody = GetComponent<RigidBody2DComponent>();
		}

		void OnDestroy()
		{
		}

		void OnUpdate(TimeStep ts)
		{
			Movement(ts);
		}

		private void Movement(TimeStep ts)
		{
			if (Input.KeyPressed(Key.L))
			{
				var transform = m_RigidBody.Transform;
				m_RigidBody.Transform = transform;
			}

			Vector2 delta = Vector2.Zero;

			if (Input.KeyPressed(Key.D))
			{
				delta += Vector2.Right * m_MovementSpeed;
			}

			if (Input.KeyPressed(Key.A))
			{
				delta += Vector2.Left * m_MovementSpeed;
			}

			var velocity = m_RigidBody.LinearVelocity;
			velocity.X = delta.X;
			m_RigidBody.LinearVelocity = velocity;

			if (UtilsKeyPressed(Key.Space, ref m_KeyControlSpace))
			{
				m_RigidBody.ApplyLinearImpulse(m_JumpForce);
			}
		}

		// until events are implemented this is the best solution for non repeating inputs
		private static bool UtilsKeyPressed(Key key, ref bool control)
		{
			if (Input.KeyPressed(key))
			{
				if (control)
				{
					control = false;
					return true;
				}
				return false;
			}
			control = true;
			return false;
		}

	}

}
