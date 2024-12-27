	
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
		public uint AirJumps;
		private uint m_AirJumpCount = 0;
		private uint m_CollishionCount = 0;

		public bool DestroyBallOnHit = false;

		public bool ShootOnPress = false;
		private bool m_WantShoot = false;
		public float ShootCooldown = 0.2f;
		private float m_ShootCooldownTimer = 0.0f;
		private bool m_Colliding => m_CollishionCount > 0;

		public Entity Camera;
		private TransformComponent m_CameraTransform;
		private Entity m_BallContainer;

		public Prefab BallPrefab;

		protected override void OnCreate()
		{
			m_CameraTransform = Camera.Transform;

			CollishionBeginEvent += CollishionBegin;
			CollishionEndEvent += CollishionEnd;

			m_BallContainer = Scene.CreateEntity("BallContainer");
			m_RigidBody = GetComponent<RigidBody2DComponent>();
		}

		protected override void OnDestroy()
		{
		}

		protected override void OnUpdate(float ts)
		{
			m_WantJump = Input.IsKeyPressed(KeyCode.Space);
			m_WantShoot = ShootOnPress ? Input.IsMousePressed(MouseButton.Left) : Input.IsMouseDown(MouseButton.Left);
			m_Sprint = Input.IsKeyDown(KeyCode.LeftShift);

			if (Input.MouseScroll != 0)
			{
				Vector3 translation = m_CameraTransform.Translation;
				translation.Z += Input.MouseScroll;
				m_CameraTransform.Translation = translation;
			}

			if (Input.IsKeyPressed(KeyCode.P, true))
				for (int y = -3; y <= 3; y++)
					for (int x = -3; x <= 3; x++)
						Scene.InstantiatePrefab(BallPrefab, m_BallContainer, new Vector3(0.0f + x, 10.0f + y, 0.0f));

			if (m_ShootCooldownTimer > 0)
				m_ShootCooldownTimer -= ts;

			if (m_WantShoot)
				Shoot();

			if (Input.IsKeyPressed(KeyCode.T))
				Translation = new Vector3(0.0f, -1.9f, 0.0f);
		}

		protected override void OnPhysicsUpdate(float fixedTimeStep)
		{
			Movement(fixedTimeStep);
		}

		protected void CollishionBegin(Entity entity)
		{
			m_RigidBody.LinearDamping = OnGroundDamping;
			m_AirJumpCount = 0;

			m_CollishionCount++;
		}

		protected void CollishionEnd(Entity entity)
		{
			m_CollishionCount--;

			if (!m_Colliding)
				m_RigidBody.LinearDamping = 0;
		}

		private void Movement(float ts)
		{
			Vector2 direction = Vector2.Zero;

			if (Input.IsKeyDown(KeyCode.D))
				direction += Vector2.Right;

			if (Input.IsKeyDown(KeyCode.A))
				direction += Vector2.Left;

			if (direction != Vector2.Zero)
			{
				var velocity = m_RigidBody.LinearVelocity;
				velocity += Acceleration * direction * ts;
				if (!m_Sprint)
				{
					velocity.X = Mathf.Clamp(velocity.X, -MaxMovementSpeed, MaxMovementSpeed);
				}
				m_RigidBody.LinearVelocity = velocity;
				m_RigidBody.LinearDamping = 0;
			}
			else if (true && m_Colliding)
			{
				m_RigidBody.LinearDamping = OnGroundDamping;
			}

			if (m_AirJumpCount < AirJumps && m_WantJump && !m_Colliding)
			{
				m_RigidBody.LinearVelocity = new Vector2(m_RigidBody.LinearVelocity.X, 0.0f);
				m_RigidBody.ApplyForce(Vector2.Up * JumpForce, PhysicsForce2DType.Impulse);
				m_AirJumpCount++;
				m_WantJump = false;
			}

			if (m_WantJump && m_Colliding)
			{
				m_RigidBody.LinearVelocity = new Vector2(m_RigidBody.LinearVelocity.X, 0.0f);
				m_RigidBody.ApplyForce(Vector2.Up * JumpForce, PhysicsForce2DType.Impulse);
				m_WantJump = false;
			}

		}

		private void Shoot()
		{
			if (ShootOnPress || m_ShootCooldownTimer <= 0.0f)
			{
				m_ShootCooldownTimer = ShootCooldown;

				var direction = Vector2.Normalize((Vector2)Input.MousePos - (Vector2)Application.Size * 0.5f);
				direction.Y = -direction.Y;



#if false
				Bullet bullet = Instantiate<Bullet>("Bullet");
				bullet.DestroyOnHit = DestroyBallOnHit;
				var rigidBody = bullet.GetComponent<RigidBody2DComponent>();
				rigidBody.Position = m_RigidBody.Position + direction;
				rigidBody.ApplyForce(direction * 10000.0f, PhysicsForce2DType.Force);
#endif
			}
		}

	}

}
