
namespace Shark
{

	public class Component
	{
		public Entity Entity { get; internal set; }
	}

	public class TagComponent : Component
	{
		public string Tag
		{
			get => InternalCalls.TagComponent_GetTag(Entity.ID);
			set => InternalCalls.TagComponent_SetTag(Entity.ID, value);
		}
	}

	public class TransformComponent : Component
	{
		public Vector3 Translation
		{
			get
			{
				InternalCalls.TransformComponent_GetTranslation(Entity.ID, out Vector3 translation);
				return translation;
			}
			set => InternalCalls.TransformComponent_SetTranslation(Entity.ID, ref value);
		}

		public Vector3 Rotation
		{
			get
			{
				InternalCalls.TransformComponent_GetRotation(Entity.ID, out Vector3 rotation);
				return rotation;
			}
			set => InternalCalls.TransformComponent_SetRotation(Entity.ID, ref value);
		}

		public Vector3 Scale
		{
			get
			{
				InternalCalls.TransformComponent_GetScale(Entity.ID, out Vector3 scale);
				return scale;
			}
			set => InternalCalls.TransformComponent_SetScale(Entity.ID, ref value);
		}

		public Transform LocalTransform
		{
			get
			{
				InternalCalls.TransformComponent_GetLocalTransform(Entity.ID, out var transform);
				return transform;
			}
			set => InternalCalls.TransformComponent_SetLocalTransform(Entity.ID, ref value);
		}
		
		public Transform WorldTransform
		{
			get
			{
				InternalCalls.TransformComponent_GetWorldTransform(Entity.ID, out var transform);
				return transform;
			}
			set => InternalCalls.TransformComponent_SetWorldTransform(Entity.ID, ref value);
		}

	}

	public class CameraComponent : Component
	{
		public enum ProjectionType
		{
			None = 0,
			Perspective, Orthographic
		};

		public Matrix4 Projection
		{
			get
			{
				InternalCalls.CameraComponent_GetProjection(Entity.ID, out Matrix4 projection);
				return projection;
			}
			set => InternalCalls.CameraComponent_SetProjection(Entity.ID, ref value);
		}

		public ProjectionType Type
		{
			get => InternalCalls.CameraComponent_GetProjectionType(Entity.ID);
			set => InternalCalls.CameraComponent_SetProjectionType(Entity.ID, value);
		}

		public void SetPerspective(float aspectratio, float fov, float clipnear, float clipfar)
			=> InternalCalls.CameraComponent_SetPerspective(Entity.ID, aspectratio, fov, clipnear, clipfar);
		public void SetOrthographic(float aspectratio, float zoom, float clipnear, float clipfar)
			=> InternalCalls.CameraComponent_SetOrthographic(Entity.ID, aspectratio, zoom, clipnear, clipfar);

		public float Aspectratio
		{
			get => InternalCalls.CameraComponent_GetAspectratio(Entity.ID);
			set => InternalCalls.CameraComponent_SetAspectratio(Entity.ID, value);
		}
		public float PerspectiveFOV
		{
			get => InternalCalls.CameraComponent_GetPerspectiveFOV(Entity.ID);
			set => InternalCalls.CameraComponent_SetPerspectiveFOV(Entity.ID, value);
		}
		public float PerspectiveNear
		{
			get => InternalCalls.CameraComponent_GetPerspectiveNear(Entity.ID);
			set => InternalCalls.CameraComponent_SetPerspectiveNear(Entity.ID, value);
		}
		public float PerspectiveFar
		{
			get => InternalCalls.CameraComponent_GetPerspectiveFar(Entity.ID);
			set => InternalCalls.CameraComponent_SetPerspectiveFar(Entity.ID, value);
		}

		public float OrthographicZoom
		{
			get => InternalCalls.CameraComponent_GetOrthographicZoom(Entity.ID);
			set => InternalCalls.CameraComponent_SetOrthographicZoom(Entity.ID, value);
		}
		public float OrthographicNear
		{
			get => InternalCalls.CameraComponent_GetOrthographicNear(Entity.ID);
			set => InternalCalls.CameraComponent_SetOrthographicNear(Entity.ID, value);
		}
		public float OrthographicFar
		{
			get => InternalCalls.CameraComponent_GetOrthographicFar(Entity.ID);
			set => InternalCalls.CameraComponent_SetOrthographicFar(Entity.ID, value);
		}
	}

	public class SpriteRendererComponent : Component
	{
		public Color Color
		{
			get
			{
				InternalCalls.SpriteRendererComponent_GetColor(Entity.ID, out Color color);
				return color;
			}
			set => InternalCalls.SpriteRendererComponent_SetColor(Entity.ID, ref value);
		}

		public AssetHandle TextureHandle
		{
			get => InternalCalls.SpriteRendererComponent_GetTextureHandle(Entity.ID);
			set => InternalCalls.SpriteRendererComponent_SetTextureHandle(Entity.ID, value);
		}

		public float TilingFactor
		{
			get => InternalCalls.SpriteRendererComponent_GetTilingFactor(Entity.ID);
			set => InternalCalls.SpriteRendererComponent_SetTilingFactor(Entity.ID, value);
		}
	}

	public class CircleRendererComponent : Component
	{
		public Color Color
		{
			get
			{
				InternalCalls.CircleRendererComponent_GetColor(Entity.ID, out Color color);
				return color;
			}
			set => InternalCalls.CircleRendererComponent_SetColor(Entity.ID, ref value);
		}

		public float Thickness
		{
			get => InternalCalls.CircleRendererComponent_GetThickness(Entity.ID);
			set => InternalCalls.CircleRendererComponent_SetThickness(Entity.ID, value);
		}

		public float Fade
		{
			get => InternalCalls.CircleRendererComponent_GetFade(Entity.ID);
			set => InternalCalls.CircleRendererComponent_SetFade(Entity.ID, value);
		}
	}

	public enum PhysicsForce2DType
	{
		Force = 0,
		Impulse = 1
	};

	public enum RigidBody2DType
	{
		None = 0,
		Static,
		Dynamic,
		Kinematic,
	};

	public class RigidBody2DComponent : Component
	{
		public RigidBody2DType BodyType
		{
			get => InternalCalls.RigidBody2DComponent_GetBodyType(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetBodyType(Entity.ID, value);
		}

		public RigidBody2DTransform Transform
		{
			get
			{
				InternalCalls.RigidBody2DComponent_GetTransform(Entity.ID, out var transform);
				return transform;
			}
			set => InternalCalls.RigidBody2DComponent_SetTransform(Entity.ID, ref value);
		}

		public Vector2 Position
		{
			get
			{
				InternalCalls.RigidBody2DComponent_GetTransform(Entity.ID, out var transform);
				return transform.Position;
			}
			set => InternalCalls.RigidBody2DComponent_SetPosition(Entity.ID, ref value);
		}

		public float Rotation
		{
			get
			{
				InternalCalls.RigidBody2DComponent_GetTransform(Entity.ID, out var transform);
				return transform.Rotation;
			}
			set => InternalCalls.RigidBody2DComponent_SetRotation(Entity.ID, value);
		}

		public Vector2 LocalCenter
		{
			get
			{
				InternalCalls.RigidBody2DComponent_GetLocalCenter(Entity.ID, out Vector2 localCenter);
				return localCenter;
			}
		}

		public Vector2 WorldCenter
		{
			get
			{
				InternalCalls.RigidBody2DComponent_GetWorldCenter(Entity.ID, out var worldCenter);
				return worldCenter;
			}
		}

		public Vector2 LinearVelocity
		{
			get
			{
				InternalCalls.RigidBody2DComponent_GetLinearVelocity(Entity.ID, out var linearVelocity);
				return linearVelocity;
			}
			set => InternalCalls.RigidBody2DComponent_SetLinearVelocity(Entity.ID, ref value);
		}

		public float AngularVelocity
		{
			get => InternalCalls.RigidBody2DComponent_GetAngularVelocity(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetAngularVelocity(Entity.ID, value);
		}

		public float GravityScale
		{
			get => InternalCalls.RigidBody2DComponent_GetGravityScale(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetGravityScale(Entity.ID, value);
		}

		public float LinearDamping
		{
			get => InternalCalls.RigidBody2DComponent_GetLinearDamping(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetLinearDamping(Entity.ID, value);
		}

		public float AngularDamping
		{
			get => InternalCalls.RigidBody2DComponent_GetAngularDamping(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetAngularDamping(Entity.ID, value);
		}

		public bool Bullet
		{
			get => InternalCalls.RigidBody2DComponent_IsBullet(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetBullet(Entity.ID, value);
		}

		public bool SleepingAllowed
		{
			get => InternalCalls.RigidBody2DComponent_IsSleepingAllowed(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetSleepingAllowed(Entity.ID, value);
		}

		public bool Awake
		{
			get => InternalCalls.RigidBody2DComponent_IsAwake(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetAwake(Entity.ID, value);
		}

		public bool Enabled
		{
			get => InternalCalls.RigidBody2DComponent_IsEnabled(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetEnabled(Entity.ID, value);
		}

		public bool FixedRotation
		{
			get => InternalCalls.RigidBody2DComponent_IsFixedRotation(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetFixedRotation(Entity.ID, value);
		}

		public void ApplyForce(Vector2 force, Vector2 point, PhysicsForce2DType forceType = PhysicsForce2DType.Force)
			=> InternalCalls.RigidBody2DComponent_ApplyForce(Entity.ID, ref force, ref point, forceType);

		public void ApplyForce(Vector2 force, PhysicsForce2DType forceType = PhysicsForce2DType.Force)
			=> InternalCalls.RigidBody2DComponent_ApplyForceToCenter(Entity.ID, ref force, forceType);

		public void ApplyTorque(float torque, PhysicsForce2DType forceType = PhysicsForce2DType.Force)
			=> InternalCalls.RigidBody2DComponent_ApplyTorque(Entity.ID, torque, forceType);
	}

	public abstract class Collider2D : Component
	{
		public abstract bool Sensor { get; set; }
		public abstract float Density { get; set; }
		public abstract float Friction { get; set; }
		public abstract float Restitution { get; set; }
		public abstract float RestitutionThreshold { get; set; }
	}

	public class BoxCollider2DComponent : Collider2D
	{
		public override bool Sensor
		{
			get => InternalCalls.BoxCollider2DComponent_IsSensor(Entity.ID);
			set => InternalCalls.BoxCollider2DComponent_SetSensor(Entity.ID, value);
		}

		public override float Density
		{
			get => InternalCalls.BoxCollider2DComponent_GetDensity(Entity.ID);
			set => InternalCalls.BoxCollider2DComponent_SetDensity(Entity.ID, value);
		}

		public override float Friction
		{
			get => InternalCalls.BoxCollider2DComponent_GetFriction(Entity.ID);
			set => InternalCalls.BoxCollider2DComponent_SetFriction(Entity.ID, value);
		}

		public override float Restitution
		{
			get => InternalCalls.BoxCollider2DComponent_GetRestitution(Entity.ID);
			set => InternalCalls.BoxCollider2DComponent_SetRestitution(Entity.ID, value);
		}

		public override float RestitutionThreshold
		{
			get => InternalCalls.BoxCollider2DComponent_GetRestitutionThreshold(Entity.ID);
			set => InternalCalls.BoxCollider2DComponent_SetRestitutionThreshold(Entity.ID, value);
		}

		public Vector2 Size
		{
			get
			{
				InternalCalls.BoxCollider2DComponent_GetSize(Entity.ID, out var size);
				return size;
			}
			set => InternalCalls.BoxCollider2DComponent_SetSize(Entity.ID, ref value);
		}

		public Vector2 Offset
		{
			get
			{
				InternalCalls.BoxCollider2DComponent_GetOffset(Entity.ID, out var offset);
				return offset;
			}
			set => InternalCalls.BoxCollider2DComponent_SetOffset(Entity.ID, ref value);
		}

		public float Rotation
		{
			get => InternalCalls.BoxCollider2DComponent_GetRotation(Entity.ID);
			set => InternalCalls.BoxCollider2DComponent_SetRotation(Entity.ID, value);
		}
	}

	public class CircleCollider2DComponent : Collider2D
	{
		public override bool Sensor
		{
			get => InternalCalls.CircleCollider2DComponent_IsSensor(Entity.ID);
			set => InternalCalls.CircleCollider2DComponent_SetSensor(Entity.ID, value);
		}

		public override float Density
		{
			get => InternalCalls.CircleCollider2DComponent_GetDensity(Entity.ID);
			set => InternalCalls.CircleCollider2DComponent_SetDensity(Entity.ID, value);
		}

		public override float Friction
		{
			get => InternalCalls.CircleCollider2DComponent_GetFriction(Entity.ID);
			set => InternalCalls.CircleCollider2DComponent_SetFriction(Entity.ID, value);
		}

		public override float Restitution
		{
			get => InternalCalls.CircleCollider2DComponent_GetRestitution(Entity.ID);
			set => InternalCalls.CircleCollider2DComponent_SetRestitution(Entity.ID, value);
		}

		public override float RestitutionThreshold
		{
			get => InternalCalls.CircleCollider2DComponent_GetRestitutionThreshold(Entity.ID);
			set => InternalCalls.CircleCollider2DComponent_SetRestitutionThreshold(Entity.ID, value);
		}

		public float Radius
		{
			get => InternalCalls.CircleCollider2DComponent_GetRadius(Entity.ID);
			set => InternalCalls.CircleCollider2DComponent_SetRadius(Entity.ID, value);
		}

		public Vector2 Offset
		{
			get
			{
				InternalCalls.CircleCollider2DComponent_GetOffset(Entity.ID, out var offset);
				return offset;
			}
			set => InternalCalls.CircleCollider2DComponent_SetOffset(Entity.ID, value);
		}

		public float Rotation
		{
			get => InternalCalls.CircleCollider2DComponent_GetRotation(Entity.ID);
			set => InternalCalls.CircleCollider2DComponent_SetRotation(Entity.ID, value);
		}
	}

}
