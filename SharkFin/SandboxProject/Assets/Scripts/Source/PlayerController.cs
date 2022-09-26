
using Shark;

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

		private Entity m_BallTemplate;

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
			m_BallTemplate = Scene.GetEntityByTag("BallTemplate");

			m_RigidBody = GetComponent<RigidBody2DComponent>();
		}

		protected override void OnDestroy()
		{
		}

		protected override void OnUpdate(float ts)
		{
			m_ShouldJump = Input.IsKeyPressed(Key.Space);
			m_WantShoot = Input.IsMouseButtonPressed(MouseButton.Left);

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
			m_CollishionCount++;

			Entity entity = collider.Entity;

			if (entity is PlatformScript)
				m_ApplyAdditionalGravity = true;
		}

		protected override void OnCollishionEnd(Collider2D collider)
		{
			m_CollishionCount--;

			Entity entity = collider.Entity;

			if (entity is PlatformScript)
				m_ApplyAdditionalGravity = false;
		}

		private void Movement(float ts)
		{
			Vector2 delta = Vector2.Zero;

			if (Input.IsKeyPressed(Key.D))
				delta += Vector2.Right;

			if (Input.IsKeyPressed(Key.A))
				delta += Vector2.Left;


			if (Input.IsKeyPressed(Key.Shift))
				delta.X *= 2.0f;

			var velocity = m_RigidBody.LinearVelocity;
			delta *= MovementSpeed * ts * 1000.0f;
			velocity.X = delta.X;
			m_RigidBody.LinearVelocity = velocity;

			if (m_ShouldJump && m_Colliding)
			{
				m_RigidBody.LinearVelocity = new Vector2(m_RigidBody.LinearVelocity.X, 0.0f);
				m_RigidBody.ApplyForce(Vector2.Up * m_JumpForce, PhysicsForce2DType.Impulse);
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
				bullet.DestroyOnHit = false;
				var rigidBody = bullet.GetComponent<RigidBody2DComponent>();
				rigidBody.Position = m_RigidBody.Position + direction;
				rigidBody.ApplyForce(direction * 10000.0f, PhysicsForce2DType.Force);
				Log.Info("Bullet Shot! Direction {0}", direction);
				Log.Warn("TestTestTest\nHalloHalloHallo\nTagTagTag");
			}
		}

		private void DoError(string fmt, string str)
		{
			DoError(fmt + str, str);
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
