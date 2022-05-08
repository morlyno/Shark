
namespace Shark
{
	public enum PhysicsForce2DType
	{
		Force = 0,
		Impulse = 1
	};

	public enum RigidBody2DType
	{
		Static = 0,
		Dynamic = 1,
		Kinematic = 2
	};

	public class RigidBody2DComponent : Component
	{
		private readonly System.IntPtr m_NativeHandle;

		public RigidBody2DType BodyType
		{
			get => InternalCalls.RigidBody2DComponent_GetBodyType(m_NativeHandle);
			set => InternalCalls.RigidBody2DComponent_SetBodyType(m_NativeHandle, value);
		}

		public RigidBody2DTransform Transform
		{
			get => InternalCalls.RigidBody2DComponent_GetTransform(m_NativeHandle);
			set => InternalCalls.RigidBody2DComponent_SetTransform(m_NativeHandle, ref value);
		}
		
		public Vector2 Position
		{
			get => InternalCalls.RigidBody2DComponent_GetTransform(m_NativeHandle).Position;
			set => InternalCalls.RigidBody2DComponent_SetPosition(m_NativeHandle, value);
		}

		public float Rotation
		{
			get => InternalCalls.RigidBody2DComponent_GetTransform(m_NativeHandle).Rotation;
			set => InternalCalls.RigidBody2DComponent_SetRotation(m_NativeHandle, value);
		}

		public Vector2 LocalCenter
		{
			get => InternalCalls.RigidBody2DComponent_GetLocalCenter(m_NativeHandle);
		}

		public Vector2 WorldCenter
		{
			get => InternalCalls.RigidBody2DComponent_GetWorldCenter(m_NativeHandle);
		}

		public Vector2 LinearVelocity
		{
			get => InternalCalls.RigidBody2DComponent_GetLinearVelocity(m_NativeHandle);
			set => InternalCalls.RigidBody2DComponent_SetLinearVelocity(m_NativeHandle, ref value);
		}

		public float AngularVelocity
		{
			get => InternalCalls.RigidBody2DComponent_GetAngularVelocity(m_NativeHandle);
			set => InternalCalls.RigidBody2DComponent_SetAngularVelocity(m_NativeHandle, value);
		}

		public float GravityScale
		{
			get => InternalCalls.RigidBody2DComponent_GetGravityScale(m_NativeHandle);
			set => InternalCalls.RigidBody2DComponent_SetGravityScale(m_NativeHandle, value);
		}

		public float LinearDamping
		{
			get => InternalCalls.RigidBody2DComponent_GetLinearDamping(m_NativeHandle);
			set => InternalCalls.RigidBody2DComponent_SetLinearDamping(m_NativeHandle, value);
		}

		public float AngularDamping
		{
			get => InternalCalls.RigidBody2DComponent_GetAngularDamping(m_NativeHandle);
			set => InternalCalls.RigidBody2DComponent_SetAngularDamping(m_NativeHandle, value);
		}

		public bool Bullet
		{
			get => InternalCalls.RigidBody2DComponent_IsBullet(m_NativeHandle);
			set => InternalCalls.RigidBody2DComponent_SetBullet(m_NativeHandle, value);
		}

		public bool SleepingAllowed
		{
			get => InternalCalls.RigidBody2DComponent_IsSleepingAllowed(m_NativeHandle);
			set => InternalCalls.RigidBody2DComponent_SetSleepingAllowed(m_NativeHandle, value);
		}

		public bool Awake
		{
			get => InternalCalls.RigidBody2DComponent_IsAwake(m_NativeHandle);
			set => InternalCalls.RigidBody2DComponent_SetAwake(m_NativeHandle, value);
		}

		public bool Enabled
		{
			get => InternalCalls.RigidBody2DComponent_IsEnabled(m_NativeHandle);
			set => InternalCalls.RigidBody2DComponent_SetEnabled(m_NativeHandle, value);
		}

		public bool FixedRotation
		{
			get => InternalCalls.RigidBody2DComponent_IsFixedRotation(m_NativeHandle);
			set => InternalCalls.RigidBody2DComponent_SetFixedRotation(m_NativeHandle, value);
		}

		public void ApplyForce(Vector2 force, Vector2 point, PhysicsForce2DType forceType = PhysicsForce2DType.Force)
		{
			InternalCalls.RigidBody2DComponent_ApplyForce(m_NativeHandle, ref force, ref point, forceType);
		}
		public void ApplyForce(Vector2 force, PhysicsForce2DType forceType = PhysicsForce2DType.Force)
		{
			InternalCalls.RigidBody2DComponent_ApplyForceToCenter(m_NativeHandle, ref force, forceType);
		}
		public void ApplyTorque(float torque, PhysicsForce2DType forceType = PhysicsForce2DType.Force)
		{
			InternalCalls.RigidBody2DComponent_ApplyTorque(m_NativeHandle, torque, forceType);
		}


		public RigidBody2DComponent(UUID owner)
			: base(owner)
		{
			m_NativeHandle = InternalCalls.RigidBody2DComponent_GetNativeHandle(m_EntityUUID);
		}
	}

}
