
using Shark;
using Shark.KeyEvents;
using Shark.MouseEvents;
using System.Collections.Generic;

namespace Sandbox
{

	public class PlayerController : Entity
	{
		private bool m_CanJump = true;
		private bool m_CanDoubleJump = true;

		// Movement
		private float m_MovementSpeed;
		private float m_JumpVelocity = 7.5f;
		private RigidBody2DComponent m_RigidBody;
		private BoxCollider2DComponent m_BoxCollider;

		private Entity m_CameraEntity = null;

		private Entity m_BallTemplate;
		private TimeStep m_Time = 0;
		private bool m_AutoSpawnBalls = false;

		protected override void OnCreate()
		{
			m_BallTemplate = Scene.GetEntityByTag("BallTemplate");

			m_RigidBody = GetComponent<RigidBody2DComponent>();
			m_BoxCollider = GetComponent<BoxCollider2DComponent>();

			m_CameraEntity = Scene.GetActiveCameraEntity();

			EventHandler.OnKeyPressed += OnKeyPressed;
			EventHandler.OnMouseScrolled += OnMouseScrolled;
		}

		protected override void OnDestroy()
		{
			EventHandler.OnKeyPressed -= OnKeyPressed;
			EventHandler.OnMouseScrolled -= OnMouseScrolled;
		}

		protected override void OnUpdate(TimeStep ts)
		{
			Movement(ts);

			if (Input.KeyPressed(Key.T))
			{
				CreateBall();
			}
			else if (m_AutoSpawnBalls)
			{
				m_Time += ts;
				if (m_Time >= TimeStep.Sec(1.0f))
				{
					CreateBall();
					m_Time = 0;
				}
			}

			if (m_CameraEntity != null)
			{
				var translation = m_CameraEntity.Transform.Translation;
				translation.x = Transform.Translation.x;
				translation.y = Transform.Translation.y;
				m_CameraEntity.Transform.Translation = translation;
			}

		}

		void OnKeyPressed(KeyPressedEvent e)
		{
			switch (e.Key)
			{
				case Key.U:
				{
					CreateBall();
					break;
				}
				case Key.I:
				{
					m_BoxCollider.Restitution = 0.7f;
					break;
				}
				case Key.O:
				{
					m_BoxCollider.Restitution = 0.0f;
					break;
				}
				case Key.Space:
				{
					if (e.IsRepeat)
						break;

					if (m_CanJump)
					{
						var vel = m_RigidBody.LinearVelocity;
						vel.Y = m_JumpVelocity;
						m_RigidBody.LinearVelocity = vel;
						m_CanJump = false;
					}
					else if (m_CanDoubleJump)
					{
						var vel = m_RigidBody.LinearVelocity;
						vel.Y = m_JumpVelocity;
						m_RigidBody.LinearVelocity = vel;
						m_CanDoubleJump = false;
					}
					break;
				}
				case Key.P:
				{
					if (!e.IsRepeat)
						m_AutoSpawnBalls = !m_AutoSpawnBalls;
					break;
				}

			}

		}

		void OnMouseScrolled(MouseScrolledEvent e)
		{
			var translation = m_CameraEntity.Transform.Translation;
			translation.z += e.Delta;
			m_CameraEntity.Transform.Translation = translation;
		}

		protected override void OnCollishionBegin(Entity entity)
		{
			m_CanJump = true;
			m_CanDoubleJump = true;
		}

		protected override void OnCollishionEnd(Entity entity)
		{
			m_CanJump = false;
		}

		private void Movement(TimeStep ts)
		{
			//MovementForce(ts);
			MovementLinearVelocity(ts);
		}

		private void MovementLinearVelocity(TimeStep ts)
		{
			m_MovementSpeed = 5.0f;
			if (Input.KeyPressed(Key.Shift))
			{
				m_MovementSpeed = 10.0f;
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
		}

		private void MovementForce(TimeStep ts)
		{
			if (Input.KeyPressed(Key.LeftArrow))
				m_RigidBody.ApplyForce(Vector2.Left * 1500.0f);

			if (Input.KeyPressed(Key.RightArrow))
				m_RigidBody.ApplyForce(Vector2.Right * 1500.0f);
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
