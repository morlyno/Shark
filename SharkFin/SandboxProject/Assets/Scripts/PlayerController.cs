
using Shark;
using Shark.KeyEvents;
using Shark.MouseEvents;
using System.Collections.Generic;

namespace Sandbox
{

	public class PlayerController : Entity
	{
		private uint m_CollishionCount = 0;
		private bool m_ShouldJump = false;

		// Movement
		public float MovementSpeed = 8.0f; // m/s;
		private float m_JumpForce = 18.0f;
		private RigidBody2DComponent m_RigidBody;

		private Entity m_ActiveCamera = null;

		private Entity m_BallTemplate;
		private TimeStep m_Time = 0;
		private bool m_AutoSpawnBalls = false;

		private bool m_ShoudShoot = false;
		private bool m_AutoShoot = false;

		private bool m_Colliding => m_CollishionCount > 0;

		protected override void OnCreate()
		{
			m_BallTemplate = Scene.GetEntityByTag("BallTemplate");

			m_RigidBody = GetComponent<RigidBody2DComponent>();

			m_ActiveCamera = Scene.GetActiveCamera();

			EventHandler.OnKeyPressed += OnKeyPressed;
			EventHandler.OnMouseScrolled += OnMouseScrolled;
			EventHandler.OnMouseButtonPressed += (e) => { if (e.Button == MouseButton.Left) m_ShoudShoot = true; };
		}

		protected override void OnDestroy()
		{
			EventHandler.OnKeyPressed -= OnKeyPressed;
			EventHandler.OnMouseScrolled -= OnMouseScrolled;
		}

		protected override void OnUpdate(TimeStep ts)
		{
			m_ShouldJump = Input.IsKeyPressed(Key.Space);

			if (m_AutoSpawnBalls)
			{
				m_Time += ts;
				if (m_Time >= TimeStep.Sec(1.0f))
				{
					CreateBall();
					m_Time = 0;
				}
			}

			if (m_ShoudShoot || m_AutoShoot && Input.IsMouseButtonPressed(MouseButton.Left))
			{
				var direction = Vector2.Normalize((Vector2)Input.GetMousePos() - (Vector2)Application.Size * 0.5f);
				direction.Y = -direction.Y;

				Entity bullet = Scene.Instantiate<Bullet>("Bullet");
				var rigidBody = bullet.GetComponent<RigidBody2DComponent>();
				rigidBody.Position = m_RigidBody.Position + direction;
				rigidBody.ApplyForce(direction * 10.0f, PhysicsForce2DType.Impulse);
				m_ShoudShoot = false;
			}

		}

		protected override void OnPhysicsUpdate(TimeStep fixedTimeStep)
		{
			Movement(fixedTimeStep);
		}

		void OnKeyPressed(KeyPressedEvent e)
		{
			switch (e.KeyCode)
			{
				case Key.U:
				{
					CreateBall();
					break;
				}
				case Key.Space:
				{
					if (e.IsRepeat)
						break;

					m_ShouldJump = true;
					break;
				}
				case Key.P:
				{
					if (!e.IsRepeat)
						m_AutoSpawnBalls = !m_AutoSpawnBalls;
					break;
				}
				case Key.F:
				{
					m_AutoShoot = !m_AutoShoot;
					break;
				}

			}

		}

		void OnMouseScrolled(MouseScrolledEvent e)
		{
			var translation = m_ActiveCamera.Transform.Translation;
			translation.z += e.Delta;
			m_ActiveCamera.Transform.Translation = translation;
		}

		protected override void OnCollishionBegin(Collider2D collider)
		{
			m_CollishionCount++;
		}

		protected override void OnCollishionEnd(Collider2D collider)
		{
			m_CollishionCount--;
		}

		private void Movement(TimeStep ts)
		{
			Vector2 delta = Vector2.Zero;

			if (Input.IsKeyPressed(Key.D))
				delta += Vector2.Right;

			if (Input.IsKeyPressed(Key.A))
				delta += Vector2.Left;


			if (Input.IsKeyPressed(Key.Shift))
				delta.X *= 2.0f;

			var velocity = m_RigidBody.LinearVelocity;
			delta *= MovementSpeed * ts.MilliSeconds;
			velocity.X = delta.X;
			m_RigidBody.LinearVelocity = velocity;

			if (m_ShouldJump && m_Colliding)
			{
				m_RigidBody.LinearVelocity = new Vector2(m_RigidBody.LinearVelocity.X, 0.0f);
				m_RigidBody.ApplyForce(Vector2.Up * m_JumpForce, PhysicsForce2DType.Impulse);
			}
		}

		private void CreateBall()
		{
			var ball = Scene.CloneEntity(m_BallTemplate);
			ball.Name = "Ball";
			var rigidBody = ball.GetComponent<RigidBody2DComponent>();
			rigidBody.Position = new Vector2(0.0f, 10.0f);
			rigidBody.Enabled = true;
		}

	}

}
