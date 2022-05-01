
namespace Shark
{

	#region Component

	public class Component
	{
		protected readonly UUID m_Owner;

		public Component(UUID owner)
		{
			m_Owner = owner;
		}
	}

	#endregion

	#region TranformComponent

	public class TransformComponent : Component
	{
		public Vector3 Translation
		{
			get => InternalCalls.TransformComponent_GetTranslation(m_Owner);
			set => InternalCalls.TransformComponent_SetTranslation(m_Owner, value);
		}

		public Vector3 Rotation
		{
			get => InternalCalls.TransformComponent_GetRotation(m_Owner);
			set => InternalCalls.TransformComponent_SetRotation(m_Owner, value);
		}

		public Vector3 Scaling
		{
			get => InternalCalls.TransformComponent_GetScaling(m_Owner);
			set => InternalCalls.TransformComponent_SetScaling(m_Owner, value);
		}

		public TransformComponent(UUID owner)
			: base(owner)
		{
		}
	}

	#endregion

	#region SpriteRendererComponent

	public class SpriteRendererComponent : Component
	{
		public Color Color
		{
			get => InternalCalls.SpriteRendererComponent_GetColor(m_Owner);
			set => InternalCalls.SpriteRendererComponent_SetColor(m_Owner, value);
		}

		public UUID TextureHandle
		{
			get => InternalCalls.SpriteRendererComponent_GetTextureHandle(m_Owner);
			set => InternalCalls.SpriteRendererComponent_SetTextureHandle(m_Owner, value);
		}
		
		public float TilingFactor
		{
			get => InternalCalls.SpriteRendererComponent_GetTilingFactor(m_Owner);
			set => InternalCalls.SpriteRendererComponent_SetTilingFactor(m_Owner, value);
		}

		public SpriteRendererComponent(UUID owner)
			: base(owner)
		{
		}
	}

	#endregion

	#region CameraComponent

	public class CameraComponent : Component
	{
		public enum ProjectionType
		{
			Perspective, Orthographic
		};

		public Matrix4 Projection
		{
			get => InternalCalls.CameraComponent_GetProjection(m_Owner);
			set => InternalCalls.CameraComponent_SetProjection(m_Owner, value);
		}

		public ProjectionType Type
		{
			get => InternalCalls.CameraComponent_GetProjectionType(m_Owner);
			set => InternalCalls.CameraComponent_SetProjectionType(m_Owner, value);
		}

		public void SetPerspective(float aspectratio, float fov, float clipnear, float clipfar) => InternalCalls.CameraComponent_SetPerspective(m_Owner, aspectratio, fov, clipnear, clipfar);
		public void SetOrthographic(float aspectratio, float zoom, float clipnear, float clipfar) => InternalCalls.CameraComponent_SetOrthographic(m_Owner, aspectratio, zoom, clipnear, clipfar);

		public float Aspectratio
		{
			get => InternalCalls.CameraComponent_GetAspectratio(m_Owner);
			set => InternalCalls.CameraComponent_SetAspectratio(m_Owner, value);
		}
		public float PerspectiveFOV
		{
			get => InternalCalls.CameraComponent_GetPerspectiveFOV(m_Owner);
			set => InternalCalls.CameraComponent_SetPerspectiveFOV(m_Owner, value);
		}
		public float PerspectiveNear
		{
			get => InternalCalls.CameraComponent_GetPerspectiveNear(m_Owner);
			set => InternalCalls.CameraComponent_SetPerspectiveNear(m_Owner, value);
		}
		public float PerspectiveFar
		{
			get => InternalCalls.CameraComponent_GetPerspectiveFar(m_Owner);
			set => InternalCalls.CameraComponent_SetPerspectiveFar(m_Owner, value);
		}
		
		public float OrthographicZoom
		{
			get => InternalCalls.CameraComponent_GetOrthographicZoom(m_Owner);
			set => InternalCalls.CameraComponent_SetOrthographicZoom(m_Owner, value);
		}
		public float OrthographicNear
		{
			get => InternalCalls.CameraComponent_GetOrthographicNear(m_Owner);
			set => InternalCalls.CameraComponent_SetOrthographicNear(m_Owner, value);
		}
		public float OrthographicFar
		{
			get => InternalCalls.CameraComponent_GetOrthographicFar(m_Owner);
			set => InternalCalls.CameraComponent_SetOrthographicFar(m_Owner, value);
		}

		public CameraComponent(UUID owner)
			: base(owner)
		{
		}
	}

	#endregion

	#region RigidBody2DComponent

	public class RigidBody2DComponent : Component
	{
		private readonly System.IntPtr m_NativeHandle;

		public RigidBody2DTransform Transform
		{
			get => InternalCalls.RigidBody2DComponent_GetTransform(m_NativeHandle);
			set => InternalCalls.RigidBody2DComponent_SetTransform(m_NativeHandle, ref value);
		}

		public Vector2 LocalCenter => InternalCalls.RigidBody2DComponent_GetLocalCenter(m_NativeHandle);
		public Vector2 WorldCenter => InternalCalls.RigidBody2DComponent_GetWorldCenter(m_NativeHandle);

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

		public void ApplyForce(Vector2 force, Vector2 point, bool wake = true) => InternalCalls.RigidBody2DComponent_ApplyForce(m_NativeHandle, ref force, ref point, wake);
		public void ApplyForce(Vector2 force, bool wake = true) => InternalCalls.RigidBody2DComponent_ApplyForceToCenter(m_NativeHandle, ref force, wake);

		public void ApplyTorque(float torque, bool wake = true) => InternalCalls.RigidBody2DComponent_ApplyTorque(m_NativeHandle, torque, wake);

		public void ApplyLinearImpulse(Vector2 impulse, Vector2 point, bool wake = true) => InternalCalls.RigidBody2DComponent_ApplyLinearImpulse(m_NativeHandle, ref impulse, ref point, wake);
		public void ApplyLinearImpulse(Vector2 impulse, bool wake = true) => InternalCalls.RigidBody2DComponent_ApplyLinearImpulseToCenter(m_NativeHandle, ref impulse, wake);

		public void ApplyAngularImpulse(float impulse, bool wake = true) => InternalCalls.RigidBody2DComponent_ApplyAngularImpulse(m_NativeHandle, impulse, wake);


		public RigidBody2DComponent(UUID owner)
			: base(owner)
		{
			m_NativeHandle = InternalCalls.RigidBody2DComponent_GetNativeHandle(m_Owner);
		}
	}

	#endregion

}
