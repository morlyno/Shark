
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
			get
			{
				if (InternalCalls.TagComponent_GetTag(Entity.ID, out string tag))
					return tag;
				return null;
			}
			set => InternalCalls.TagComponent_SetTag(Entity.ID, value);
		}
	}

	public class TransformComponent : Component
	{
		public Vector3 Translation
		{
			get
			{
				if (InternalCalls.TransformComponent_GetTranslation(Entity.ID, out Vector3 translation))
					return translation;
				return Vector3.Zero;
			}
			set => InternalCalls.TransformComponent_SetTranslation(Entity.ID, ref value);
		}

		public Vector3 Rotation
		{
			get
			{
				if (InternalCalls.TransformComponent_GetRotation(Entity.ID, out Vector3 rotation))
					return rotation;
				return Vector3.Zero;
			}
			set => InternalCalls.TransformComponent_SetRotation(Entity.ID, ref value);
		}

		public Vector3 Scale
		{
			get
			{
				if (InternalCalls.TransformComponent_GetScale(Entity.ID, out Vector3 scale))
					return scale;
				return Vector3.One;
			}
			set => InternalCalls.TransformComponent_SetScale(Entity.ID, ref value);
		}

		public Transform LocalTransform
		{
			get
			{
				if (InternalCalls.TransformComponent_GetLocalTransform(Entity.ID, out var transform))
					return transform;
				return default;
			}
			set => InternalCalls.TransformComponent_SetLocalTransform(Entity.ID, ref value);
		}
		
		public Transform WorldTransform
		{
			get
			{
				if (InternalCalls.TransformComponent_GetWorldTransform(Entity.ID, out var transform))
					return transform;
				return default;
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
				if (InternalCalls.CameraComponent_GetProjection(Entity.ID, out Matrix4 projection))
					return Projection;
				return Matrix4.Identity;
			}
			set => InternalCalls.CameraComponent_SetProjection(Entity.ID, ref value);
		}

		public ProjectionType Type
		{
			get
			{
				if (InternalCalls.CameraComponent_GetProjectionType(Entity.ID, out ProjectionType projectionType))
					return projectionType;
				return ProjectionType.None;
			}
			set => InternalCalls.CameraComponent_SetProjectionType(Entity.ID, value);
		}

		public void SetPerspective(float aspectratio, float fov, float clipnear, float clipfar)
			=> InternalCalls.CameraComponent_SetPerspective(Entity.ID, aspectratio, fov, clipnear, clipfar);
		public void SetOrthographic(float aspectratio, float zoom, float clipnear, float clipfar)
			=> InternalCalls.CameraComponent_SetOrthographic(Entity.ID, aspectratio, zoom, clipnear, clipfar);

		public float Aspectratio
		{
			get
			{
				if (InternalCalls.CameraComponent_GetAspectratio(Entity.ID, out float aspectratio))
					return aspectratio;
				return 0.0f;
			}
			set => InternalCalls.CameraComponent_SetAspectratio(Entity.ID, value);
		}
		public float PerspectiveFOV
		{
			get
			{
				if (InternalCalls.CameraComponent_GetPerspectiveFOV(Entity.ID, out float fov))
					return fov;
				return 0.0f;
			}
			set => InternalCalls.CameraComponent_SetPerspectiveFOV(Entity.ID, value);
		}
		public float PerspectiveNear
		{
			get
			{
				if (InternalCalls.CameraComponent_GetPerspectiveNear(Entity.ID, out float near))
					return near;
				return 0.0f;
			}
			set => InternalCalls.CameraComponent_SetPerspectiveNear(Entity.ID, value);
		}
		public float PerspectiveFar
		{
			get
			{
				if (InternalCalls.CameraComponent_GetPerspectiveFar(Entity.ID, out float far))
					return far;
				return 0.0f;
			}
			set => InternalCalls.CameraComponent_SetPerspectiveFar(Entity.ID, value);
		}

		public float OrthographicZoom
		{
			get
			{
				if (InternalCalls.CameraComponent_GetOrthographicZoom(Entity.ID, out float zoom))
					return zoom;
				return 0.0f;
			}
			set => InternalCalls.CameraComponent_SetOrthographicZoom(Entity.ID, value);
		}
		public float OrthographicNear
		{
			get
			{
				if (InternalCalls.CameraComponent_GetOrthographicNear(Entity.ID, out float near))
					return near;
				return 0.0f;
			}
			set => InternalCalls.CameraComponent_SetOrthographicNear(Entity.ID, value);
		}
		public float OrthographicFar
		{
			get
			{
				if (InternalCalls.CameraComponent_GetOrthographicFar(Entity.ID, out float far))
					return far;
				return 0.0f;
			}
			set => InternalCalls.CameraComponent_SetOrthographicFar(Entity.ID, value);
		}
	}

	public class SpriteRendererComponent : Component
	{
		public Color Color
		{
			get
			{
				if (InternalCalls.SpriteRendererComponent_GetColor(Entity.ID, out Color color))
					return color;
				return Color.Black;
			}
			set => InternalCalls.SpriteRendererComponent_SetColor(Entity.ID, ref value);
		}

		public AssetHandle TextureHandle
		{
			get
			{
				if (InternalCalls.SpriteRendererComponent_GetTextureHandle(Entity.ID, out AssetHandle textureHandle))
					return textureHandle;
				return AssetHandle.Invalid;
			}
			set => InternalCalls.SpriteRendererComponent_SetTextureHandle(Entity.ID, ref value);
		}

		public float TilingFactor
		{
			get
			{
				if (InternalCalls.SpriteRendererComponent_GetTilingFactor(Entity.ID, out float tilingFactor))
					return tilingFactor;
				return 0.0f;
			}
			set => InternalCalls.SpriteRendererComponent_SetTilingFactor(Entity.ID, value);
		}
	}

	public class CircleRendererComponent : Component
	{
		public Color Color
		{
			get
			{
				if (InternalCalls.CircleRendererComponent_GetColor(Entity.ID, out Color color))
					return color;
				return Color.Black;
			}
			set => InternalCalls.CircleRendererComponent_SetColor(Entity.ID, ref value);
		}

		public float Thickness
		{
			get
			{
				if (InternalCalls.CircleRendererComponent_GetThickness(Entity.ID, out float thickness))
					return thickness;
				return 0.0f;
			}
			set => InternalCalls.CircleRendererComponent_SetThickness(Entity.ID, value);
		}

		public float Fade
		{
			get
			{
				if (InternalCalls.CircleRendererComponent_GetFade(Entity.ID, out float fade))
					return fade;
				return 0.0f;
			}
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
			get
			{
				if (InternalCalls.RigidBody2DComponent_GetBodyType(Entity.ID, out var bodyType))
					return bodyType;
				return RigidBody2DType.None;
			}
			set => InternalCalls.RigidBody2DComponent_SetBodyType(Entity.ID, value);
		}

		public RigidBody2DTransform Transform
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_GetTransform(Entity.ID, out var transform))
					return transform;
				return default;
			}
			set => InternalCalls.RigidBody2DComponent_SetTransform(Entity.ID, ref value);
		}

		public Vector2 Position
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_GetTransform(Entity.ID, out var transform))
					return transform.Position;
				return Vector2.Zero;
			}
			set => InternalCalls.RigidBody2DComponent_SetPosition(Entity.ID, ref value);
		}

		public float Rotation
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_GetTransform(Entity.ID, out var transform))
					return transform.Rotation;
				return 0.0f;
			}
			set => InternalCalls.RigidBody2DComponent_SetRotation(Entity.ID, value);
		}

		public Vector2 LocalCenter
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_GetLocalCenter(Entity.ID, out Vector2 localCenter))
					return localCenter;
				return Vector2.Zero;
			}
		}

		public Vector2 WorldCenter
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_GetWorldCenter(Entity.ID, out var worldCenter))
					return WorldCenter;
				return Vector2.Zero;
			}
		}

		public Vector2 LinearVelocity
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_GetLinearVelocity(Entity.ID, out var linearVelocity))
					return linearVelocity;
				return Vector2.Zero;
			}
			set => InternalCalls.RigidBody2DComponent_SetLinearVelocity(Entity.ID, ref value);
		}

		public float AngularVelocity
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_GetAngularVelocity(Entity.ID, out float angularVelocity))
					return AngularVelocity;
				return 0.0f;
			}
			set => InternalCalls.RigidBody2DComponent_SetAngularVelocity(Entity.ID, value);
		}

		public float GravityScale
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_GetGravityScale(Entity.ID, out float gravityScale))
					return gravityScale;
				return 0.0f;
			}
			set => InternalCalls.RigidBody2DComponent_SetGravityScale(Entity.ID, value);
		}

		public float LinearDamping
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_GetLinearDamping(Entity.ID, out float linearDamping))
					return LinearDamping;
				return 0.0f;
			}
			set => InternalCalls.RigidBody2DComponent_SetLinearDamping(Entity.ID, value);
		}

		public float AngularDamping
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_GetAngularDamping(Entity.ID, out float angularDamping))
					return angularDamping;
				return 0.0f;
			}
			set => InternalCalls.RigidBody2DComponent_SetAngularDamping(Entity.ID, value);
		}

		public bool Bullet
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_IsBullet(Entity.ID, out bool isBullet))
					return isBullet;
				return false;
			}
			set => InternalCalls.RigidBody2DComponent_SetBullet(Entity.ID, value);
		}

		public bool SleepingAllowed
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_IsSleepingAllowed(Entity.ID, out bool isSleepingAllowed))
					return isSleepingAllowed;
				return false;
			}
			set => InternalCalls.RigidBody2DComponent_SetSleepingAllowed(Entity.ID, value);
		}

		public bool Awake
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_IsAwake(Entity.ID, out bool isAwake))
					return isAwake;
				return false;
			}
			set => InternalCalls.RigidBody2DComponent_SetAwake(Entity.ID, value);
		}

		public bool Enabled
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_IsEnabled(Entity.ID, out bool isEnalbed))
					return isEnalbed;
				return false;
			}
			set => InternalCalls.RigidBody2DComponent_SetEnabled(Entity.ID, value);
		}

		public bool FixedRotation
		{
			get
			{
				if (InternalCalls.RigidBody2DComponent_IsFixedRotation(Entity.ID, out bool isFixetRotation))
					return isFixetRotation;
				return false;
			}
			set => InternalCalls.RigidBody2DComponent_SetFixedRotation(Entity.ID, value);
		}

		public void ApplyForce(Vector2 force, Vector2 point, PhysicsForce2DType forceType = PhysicsForce2DType.Force)
			=> InternalCalls.RigidBody2DComponent_ApplyForce(Entity.ID, ref force, ref point, forceType);

		public void ApplyForce(Vector2 force, PhysicsForce2DType forceType = PhysicsForce2DType.Force)
			=> InternalCalls.RigidBody2DComponent_ApplyForceToCenter(Entity.ID, ref force, forceType);

		public void ApplyTorque(float torque, PhysicsForce2DType forceType = PhysicsForce2DType.Force)
			=> InternalCalls.RigidBody2DComponent_ApplyTorque(Entity.ID, torque, forceType);
	}

	public abstract class PhysicsCollider : Component
	{
		public abstract bool Sensor { get; set; }
		public abstract float Density { get; set; }
		public abstract float Friction { get; set; }
		public abstract float Restitution { get; set; }
		public abstract float RestitutionThreshold { get; set;  }
	}

	public class BoxCollider2DComponent : PhysicsCollider
	{
		public override bool Sensor
		{
			get
			{
				if (InternalCalls.BoxCollider2DComponent_IsSensor(Entity.ID, out bool isSensor))
					return isSensor;
				return false;
			}
			set => InternalCalls.BoxCollider2DComponent_SetSensor(Entity.ID, value);
		}

		public override float Density
		{
			get
			{
				if (InternalCalls.BoxCollider2DComponent_GetDensity(Entity.ID, out float density))
					return density;
				return 0.0f;
			}
			set => InternalCalls.BoxCollider2DComponent_SetDensity(Entity.ID, value);
		}

		public override float Friction
		{
			get
			{
				if (InternalCalls.BoxCollider2DComponent_GetFriction(Entity.ID, out float friction))
					return friction;
				return 0.0f;
			}
			set => InternalCalls.BoxCollider2DComponent_SetFriction(Entity.ID, value);
		}

		public override float Restitution
		{
			get
			{
				if (InternalCalls.BoxCollider2DComponent_GetRestitution(Entity.ID, out float restitution))
					return restitution;
				return 0.0f;
			}
			set => InternalCalls.BoxCollider2DComponent_SetRestitution(Entity.ID, value);
		}

		public override float RestitutionThreshold
		{
			get
			{
				if (InternalCalls.BoxCollider2DComponent_GetRestitutionThreshold(Entity.ID, out float restitutionThreshold))
					return restitutionThreshold;
				return 0.0f;
			}
			set => InternalCalls.BoxCollider2DComponent_SetRestitutionThreshold(Entity.ID, value);
		}

		public Vector2 Size
		{
			get
			{
				if (InternalCalls.BoxCollider2DComponent_GetSize(Entity.ID, out var size))
					return size;
				return Vector2.Zero;
			}
			set => InternalCalls.BoxCollider2DComponent_SetSize(Entity.ID, ref value);
		}

		public Vector2 Offset
		{
			get
			{
				if (InternalCalls.BoxCollider2DComponent_GetOffset(Entity.ID, out var offset))
					return offset;
				return Vector2.Zero;
			}
			set => InternalCalls.BoxCollider2DComponent_SetOffset(Entity.ID, ref value);
		}

		public float Rotation
		{
			get
			{
				if (InternalCalls.BoxCollider2DComponent_GetRotation(Entity.ID, out float rotation))
					return rotation;
				return 0.0f;
			}
			set => InternalCalls.BoxCollider2DComponent_SetRotation(Entity.ID, value);
		}
	}

	public class CircleCollider2DComponent : PhysicsCollider
	{
		public override bool Sensor
		{
			get
			{
				if (InternalCalls.CircleCollider2DComponent_IsSensor(Entity.ID, out bool isSensor))
					return isSensor;
				return false;
			}
			set => InternalCalls.CircleCollider2DComponent_SetSensor(Entity.ID, value);
		}

		public override float Density
		{
			get
			{
				if (InternalCalls.CircleCollider2DComponent_GetDensity(Entity.ID, out float density))
					return density;
				return 0.0f;
			}
			set => InternalCalls.CircleCollider2DComponent_SetDensity(Entity.ID, value);
		}

		public override float Friction
		{
			get
			{
				if (InternalCalls.CircleCollider2DComponent_GetFriction(Entity.ID, out float friction))
					return friction;
				return 0.0f;
			}
			set => InternalCalls.CircleCollider2DComponent_SetFriction(Entity.ID, value);
		}

		public override float Restitution
		{
			get
			{
				if (InternalCalls.CircleCollider2DComponent_GetRestitution(Entity.ID, out float restitution))
					return restitution;
				return 0.0f;
			}
			set => InternalCalls.CircleCollider2DComponent_SetRestitution(Entity.ID, value);
		}

		public override float RestitutionThreshold
		{
			get
			{
				if (InternalCalls.CircleCollider2DComponent_GetRestitutionThreshold(Entity.ID, out float restitutionThreshold))
					return restitutionThreshold;
				return 0.0f;
			}
			set => InternalCalls.CircleCollider2DComponent_SetRestitutionThreshold(Entity.ID, value);
		}

		public float Radius
		{
			get
			{
				if (InternalCalls.CircleCollider2DComponent_GetRadius(Entity.ID, out float radius))
					return radius;
				return 0.0f;
			}
			set => InternalCalls.CircleCollider2DComponent_SetRadius(Entity.ID, value);
		}

		public Vector2 Offset
		{
			get
			{
				if (InternalCalls.CircleCollider2DComponent_GetOffset(Entity.ID, out var offset))
					return offset;
				return Vector2.Zero;
			}
			set => InternalCalls.CircleCollider2DComponent_SetOffset(Entity.ID, value);
		}

		public float Rotation
		{
			get
			{
				if (InternalCalls.CircleCollider2DComponent_GetRotation(Entity.ID, out float rotation))
					return rotation;
				return 0.0f;
			}
			set => InternalCalls.CircleCollider2DComponent_SetRotation(Entity.ID, value);
		}
	}

}
