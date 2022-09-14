
using Shark;

namespace Sandbox
{

	public class PlayerController : Entity
	{
		public float Acceleration;
		public float MaxMovementSpeed; // m/s;
		public float OnGroundDamping = 10.0f;
		public float JumpForce = 18.0f;
		private RigidBody2DComponent m_RigidBody;

		private bool m_Sprint = false;
		private bool m_WantJump = false;
		private uint m_CollishionCount = 0;


		public Entity BallTemplate;
		public bool DestroyBallOnHit = false;

		private bool m_WantShoot = false;
		private float m_ShootCooldown = 0.2f;
		private float m_ShootCooldownTimer = 0.0f;

		private bool m_Colliding => m_CollishionCount > 0;

		private uint m_InternalApplyAdditionalGravityCount = 0;
		private bool m_ApplyAdditionalGravity
		{
			get
			{
				return m_InternalApplyAdditionalGravityCount > 0;
			}
			set
			{
				if (value)
					m_InternalApplyAdditionalGravityCount++;
				else
					m_InternalApplyAdditionalGravityCount--;
			}
		}

		protected override void OnCreate()
		{
			//m_BallTemplate = Scene.GetEntityByTag("BallTemplate");

			m_RigidBody = GetComponent<RigidBody2DComponent>();
			m_RigidBody.LinearDamping = 0.5f;
			//m_RigidBody.GravityScale = 10.0f;
		}

		protected override void OnDestroy()
		{
		}

		protected override void OnUpdate(float ts)
		{
			m_WantJump = Input.IsKeyPressed(Key.Space);
			m_WantShoot = Input.IsMouseButtonPressed(MouseButton.Left);
			m_Sprint = Input.IsKeyPressed(Key.LeftShift);

			if (Input.IsKeyPressed(Key.P))
				CreateBall();

			if (m_ShootCooldownTimer > 0)
				m_ShootCooldownTimer -= ts;

			if (m_WantShoot)
				Shoot();
		}

		protected override void OnPhysicsUpdate(float fixedTimeStep)
		{
			if (m_ApplyAdditionalGravity)
			{
				var linearVelocity = m_RigidBody.LinearVelocity;
				linearVelocity.Y = Mathf.Min(Physics2D.Gravity.Y, linearVelocity.Y);
				m_RigidBody.LinearVelocity = linearVelocity;
			}
			Movement(fixedTimeStep);
		}

		protected override void OnCollishionBegin(Collider2D collider)
		{
			m_RigidBody.LinearDamping = OnGroundDamping;

			m_CollishionCount++;
			Entity entity = collider.Entity;

			if (entity is PlatformScript)
				m_ApplyAdditionalGravity = true;
		}

		protected override void OnCollishionEnd(Collider2D collider)
		{
			m_CollishionCount--;

			if (!m_Colliding)
				m_RigidBody.LinearDamping = 0;

			Entity entity = collider.Entity;

			if (entity is PlatformScript)
				m_ApplyAdditionalGravity = false;
		}

		private void Movement(float ts)
		{
			Vector2 direction = Vector2.Zero;
			bool apply = false;

			if (Input.IsKeyPressed(Key.D))
			{
				direction += Vector2.Right;
				apply = true;
			}

			if (Input.IsKeyPressed(Key.A))
			{
				direction += Vector2.Left;
				apply = true;
			}

			if (apply)
			{
				var velocity = m_RigidBody.LinearVelocity;
				velocity.X += Acceleration * direction.X * ts;
				if (!m_Sprint)
					velocity.X = Mathf.Clamp(velocity.X, -MaxMovementSpeed, MaxMovementSpeed);
				m_RigidBody.LinearVelocity = velocity;
				m_RigidBody.LinearDamping = 0;
			}
			else if (m_Colliding)
			{
				m_RigidBody.LinearDamping = OnGroundDamping;
			}

			if (m_WantJump && m_Colliding)
			{
				m_RigidBody.LinearVelocity = new Vector2(m_RigidBody.LinearVelocity.X, 0.0f);
				m_RigidBody.ApplyForce(Vector2.Up * JumpForce, PhysicsForce2DType.Impulse);
			}

		}

		private void Shoot()
		{
			if (m_ShootCooldownTimer <= 0.0f)
			{
				m_ShootCooldownTimer = m_ShootCooldown;

				var direction = Vector2.Normalize((Vector2)Input.GetMousePos() - (Vector2)Application.Size * 0.5f);
				direction.Y = -direction.Y;

				Bullet bullet = Scene.Instantiate<Bullet>("Bullet");
				bullet.DestroyOnHit = DestroyBallOnHit;
				var rigidBody = bullet.GetComponent<RigidBody2DComponent>();
				rigidBody.Position = m_RigidBody.Position + direction;
				rigidBody.ApplyForce(direction * 10000.0f, PhysicsForce2DType.Force);
			}
		}

		private void CreateBall()
		{
			var ball = Scene.CloneEntity(BallTemplate);
			ball.Name = "Ball";
			var rigidBody = ball.GetComponent<RigidBody2DComponent>();
			rigidBody.Position = new Vector2(0.0f, 10.0f);
			rigidBody.Enabled = true;
		}

	}

}
