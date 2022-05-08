
using Shark;

namespace Sandbox
{

	public class PlayerController : Entity
	{
		private bool m_CanJump = true;
		private bool m_CanDoubleJump = true;
		private bool m_SpaceKeyControl = false;
		private bool m_IKeyControl = false;
		private bool m_OKeyControl = false;
		private bool m_UKeyControl = false;

		// Movement
		private float m_MovementSpeed;
		private float m_JumpVelocity = 7.5f;
		private RigidBody2DComponent m_RigidBody;
		private BoxCollider2DComponent m_BoxCollider;

		private Entity m_CameraEntity = null;

		private Entity m_BallTemplate;
		private TimeStep m_Time = 0;

		void OnCreate()
		{
			m_BallTemplate = Scene.GetEntityByTag("BallTemplate");

			m_RigidBody = GetComponent<RigidBody2DComponent>();
			m_BoxCollider = GetComponent<BoxCollider2DComponent>();

			m_CameraEntity = Scene.GetEntityByTag("Camera");
		}

		void OnDestroy()
		{
		}

		void OnUpdate(TimeStep ts)
		{
			Movement(ts);

			if (UtilsKeyPressed(Key.U, ref m_UKeyControl))
			{
				CreateBall();
			}

			if (Input.KeyPressed(Key.T))
			{
				CreateBall();
			}
			else
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
				translation.X = Transform.Translation.X;
				translation.Y = Transform.Translation.Y;
				m_CameraEntity.Transform.Translation = translation;
			}
		}

		void OnCollishionBegin(Entity entity)
		{
			m_CanJump = true;
			m_CanDoubleJump = true;
		}
		
		void OnCollishionEnd(Entity entity)
		{
			m_CanJump = false;
		}

		private void Movement(TimeStep ts)
		{
			//MovementForce(ts);
			MovementLinearVelocity(ts);
			MovementJump(ts);

			if (UtilsKeyPressed(Key.I, ref m_IKeyControl))
			{
				m_BoxCollider.Restitution = 0.7f;
			}
			if (UtilsKeyPressed(Key.O, ref m_OKeyControl))
			{
				m_BoxCollider.Restitution = 0.0f;
			}
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

		private void MovementJump(TimeStep ts)
		{
			bool spacePressed = UtilsKeyPressed(Key.Space, ref m_SpaceKeyControl);
			if (m_CanJump && spacePressed)
			{
				var vel = m_RigidBody.LinearVelocity;
				vel.Y = m_JumpVelocity;
				m_RigidBody.LinearVelocity = vel;
				m_CanJump = false;
			}
			else if (m_CanDoubleJump && spacePressed)
			{
				var vel = m_RigidBody.LinearVelocity;
				vel.Y = m_JumpVelocity;
				m_RigidBody.LinearVelocity = vel;
				m_CanDoubleJump = false;
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
