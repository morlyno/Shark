
namespace Shark
{
	public class PhysicsCollider2D : Component
	{
		protected readonly System.IntPtr m_NativeHandle;

		public bool Sensor
		{
			get => InternalCalls.PhysicsCollider2D_IsSensor(m_NativeHandle);
			set => InternalCalls.PhysicsCollider2D_SetSensor(m_NativeHandle, value);
		}

		public float Density
		{
			get => InternalCalls.PhysicsCollider2D_GetDensity(m_NativeHandle);
			set => InternalCalls.PhysicsCollider2D_SetDensity(m_NativeHandle, value);
		}

		public float Friction
		{
			get => InternalCalls.PhysicsCollider2D_GetFriction(m_NativeHandle);
			set => InternalCalls.PhysicsCollider2D_SetFriction(m_NativeHandle, value);
		}

		public float Restitution
		{
			get => InternalCalls.PhysicsCollider2D_GetRestitution(m_NativeHandle);
			set => InternalCalls.PhysicsCollider2D_SetRestitution(m_NativeHandle, value);
		}

		public float RestitutionThreshold
		{
			get => InternalCalls.PhysicsCollider2D_GetRestitutionThreshold(m_NativeHandle);
			set => InternalCalls.PhysicsCollider2D_SetRestitutionThreshold(m_NativeHandle, value);
		}

		public PhysicsCollider2D(UUID owner, System.IntPtr nativeHandle)
			: base(owner)
		{
			m_NativeHandle = nativeHandle;
		}

		// TestPoint
		// RayCast
		// Filter
	}

}
