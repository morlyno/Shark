
using Coral.Managed.Interop;

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
			get { unsafe { return InternalCalls.TagComponent_GetTag(Entity.ID); } }
			set { unsafe { InternalCalls.TagComponent_SetTag(Entity.ID, value); } }
		}
	}

	public class TransformComponent : Component
	{
		public Vector3 Translation
		{
			get
			{
				Vector3 result = new();
				unsafe { InternalCalls.TransformComponent_GetTranslation(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe { InternalCalls.TransformComponent_SetTranslation(Entity.ID, &value); }
			}
		}

		public Vector3 Rotation
		{
			get
			{
				Vector3 result = new();
				unsafe { InternalCalls.TransformComponent_GetRotation(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe { InternalCalls.TransformComponent_SetRotation(Entity.ID, &value); }
			}
		}

		public Vector3 Scale
		{
			get
			{
				Vector3 result = new();
				unsafe { InternalCalls.TransformComponent_GetScale(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe { InternalCalls.TransformComponent_SetScale(Entity.ID, &value); }
			}
		}

		public Transform LocalTransform
		{
			get
			{
				Transform result = new();
				unsafe { InternalCalls.TransformComponent_GetLocalTransform(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe { InternalCalls.TransformComponent_SetLocalTransform(Entity.ID, &value); }
			}
		}
		
		public Transform WorldTransform
		{
			get
			{
				Transform result = new();
				unsafe { InternalCalls.TransformComponent_GetWorldTransform(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe { InternalCalls.TransformComponent_SetWorldTransform(Entity.ID, &value); }
			}
		}

	}

	public class CameraComponent : Component
	{
		public enum ProjectionType
		{
			Perspective, Orthographic
		};

		public Matrix4 Projection
		{
			get
			{
				Matrix4 result = new();
				unsafe { InternalCalls.CameraComponent_GetProjection(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe
				{
					InternalCalls.CameraComponent_SetProjection(Entity.ID, &value);
				}
			}
		}

		public ProjectionType Type
		{
			get { unsafe { return InternalCalls.CameraComponent_GetIsPerspective(Entity.ID) ? ProjectionType.Perspective : ProjectionType.Orthographic; } }
			set { unsafe { InternalCalls.CameraComponent_SetIsPerspective(Entity.ID, value == ProjectionType.Perspective); } }
		}

		public void SetPerspective(float aspectratio, float fov, float clipnear, float clipfar)
		{
			unsafe
			{
				InternalCalls.CameraComponent_SetPerspective(Entity.ID, aspectratio, fov, clipnear, clipfar);
			}
		}

		public void SetOrthographic(float aspectratio, float zoom, float clipnear, float clipfar)
		{
			unsafe
			{
				InternalCalls.CameraComponent_SetOrthographic(Entity.ID, aspectratio, zoom, clipnear, clipfar);
			}
		}

		public float Aspectratio
		{
			get { unsafe { return InternalCalls.CameraComponent_GetAspectratio(Entity.ID); } }
			set { unsafe { InternalCalls.CameraComponent_SetAspectratio(Entity.ID, value); } }
		}
		public float PerspectiveFOV
		{
			get { unsafe { return InternalCalls.CameraComponent_GetPerspectiveFOV(Entity.ID); } }
			set { unsafe { InternalCalls.CameraComponent_SetPerspectiveFOV(Entity.ID, value); } }
		}
		public float PerspectiveNear
		{
			get { unsafe { return InternalCalls.CameraComponent_GetPerspectiveNear(Entity.ID); } }
			set { unsafe { InternalCalls.CameraComponent_SetPerspectiveNear(Entity.ID, value); } }
		}
		public float PerspectiveFar
		{
			get { unsafe { return InternalCalls.CameraComponent_GetPerspectiveFar(Entity.ID); } }
			set { unsafe { InternalCalls.CameraComponent_SetPerspectiveFar(Entity.ID, value); } }
		}

		public float OrthographicZoom
		{
			get { unsafe { return InternalCalls.CameraComponent_GetOrthographicZoom(Entity.ID); } }
			set { unsafe { InternalCalls.CameraComponent_SetOrthographicZoom(Entity.ID, value); } }
		}
		public float OrthographicNear
		{
			get { unsafe { return InternalCalls.CameraComponent_GetOrthographicNear(Entity.ID); } }
			set { unsafe { InternalCalls.CameraComponent_SetOrthographicNear(Entity.ID, value); } }
		}
		public float OrthographicFar
		{
			get { unsafe { return InternalCalls.CameraComponent_GetOrthographicFar(Entity.ID); } }
			set { unsafe { InternalCalls.CameraComponent_SetOrthographicFar(Entity.ID, value); } }
		}
	}

	public class SpriteRendererComponent : Component
	{
		public Color Color
		{
			get
			{
				Color result = new();
				unsafe { InternalCalls.SpriteRendererComponent_GetColor(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe { InternalCalls.SpriteRendererComponent_SetColor(Entity.ID, &value); }
			}
		}

		public AssetHandle TextureHandle
		{
			get { unsafe { return InternalCalls.SpriteRendererComponent_GetTextureHandle(Entity.ID); } }
			set { unsafe { InternalCalls.SpriteRendererComponent_SetTextureHandle(Entity.ID, value); } }
		}

		public Vector2 TilingFactor
		{
			get
			{
				Vector2 result = new();
				unsafe { InternalCalls.SpriteRendererComponent_GetTilingFactor(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe { InternalCalls.SpriteRendererComponent_SetTilingFactor(Entity.ID, &value); }
			}
		}
	}

	public class CircleRendererComponent : Component
	{
		public Color Color
		{
			get
			{
				Color result = new();
				unsafe { InternalCalls.CircleRendererComponent_GetColor(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe { InternalCalls.CircleRendererComponent_SetColor(Entity.ID, &value); }
			}
		}

		public float Thickness
		{
			get { unsafe { return InternalCalls.CircleRendererComponent_GetThickness(Entity.ID); } }
			set { unsafe { InternalCalls.CircleRendererComponent_SetThickness(Entity.ID, value); } }
		}

		public float Fade
		{
			get { unsafe { return InternalCalls.CircleRendererComponent_GetFade(Entity.ID); } }
			set { unsafe { InternalCalls.CircleRendererComponent_SetFade(Entity.ID, value); } }
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
			get { unsafe { return InternalCalls.RigidBody2DComponent_GetBodyType(Entity.ID); } }
			set { unsafe { InternalCalls.RigidBody2DComponent_SetBodyType(Entity.ID, value); } }
		}

		public Vector2 Position
		{
			get
			{
				Vector2 result = new();
				unsafe { InternalCalls.RigidBody2DComponent_GetPosition(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe { InternalCalls.RigidBody2DComponent_SetPosition(Entity.ID, &value); }
			}
		}

		public float Rotation
		{
			get
			{
				unsafe { return InternalCalls.RigidBody2DComponent_GetRotation(Entity.ID); }
			}
			set
			{
				unsafe { InternalCalls.RigidBody2DComponent_SetRotation(Entity.ID, value); }
			}
		}

		public Vector2 LocalCenter
		{
			get
			{
				Vector2 result = new();
				unsafe { InternalCalls.RigidBody2DComponent_GetLocalCenter(Entity.ID, &result); return result; }
			}
		}

		public Vector2 WorldCenter
		{
			get
			{
				Vector2 result = new();
				unsafe { InternalCalls.RigidBody2DComponent_GetWorldCenter(Entity.ID, &result); return result; }
			}
		}

		public Vector2 LinearVelocity
		{
			get
			{
				Vector2 result = new();
				unsafe { InternalCalls.RigidBody2DComponent_GetLinearVelocity(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe { InternalCalls.RigidBody2DComponent_SetLinearVelocity(Entity.ID, &value); }
			}
		}

		public float AngularVelocity
		{
			get { unsafe { return InternalCalls.RigidBody2DComponent_GetAngularVelocity(Entity.ID); } }
			set { unsafe { InternalCalls.RigidBody2DComponent_SetAngularVelocity(Entity.ID, value); } }
		}

		public float GravityScale
		{
			get { unsafe { return InternalCalls.RigidBody2DComponent_GetGravityScale(Entity.ID); } }
			set { unsafe { InternalCalls.RigidBody2DComponent_SetGravityScale(Entity.ID, value); } }
		}

		public float LinearDamping
		{
			get { unsafe { return InternalCalls.RigidBody2DComponent_GetLinearDamping(Entity.ID); } }
			set { unsafe { InternalCalls.RigidBody2DComponent_SetLinearDamping(Entity.ID, value); } }
		}

		public float AngularDamping
		{
			get { unsafe { return InternalCalls.RigidBody2DComponent_GetAngularDamping(Entity.ID); } }
			set { unsafe { InternalCalls.RigidBody2DComponent_SetAngularDamping(Entity.ID, value); } }
		}

		public bool Bullet
		{
			get { unsafe { return InternalCalls.RigidBody2DComponent_IsBullet(Entity.ID); } }
			set { unsafe { InternalCalls.RigidBody2DComponent_SetBullet(Entity.ID, value); } }
		}

		public bool SleepingAllowed
		{
			get { unsafe { return InternalCalls.RigidBody2DComponent_IsSleepingAllowed(Entity.ID); } }
			set { unsafe { InternalCalls.RigidBody2DComponent_SetSleepingAllowed(Entity.ID, value); } }
		}

		public bool Awake
		{
			get { unsafe { return InternalCalls.RigidBody2DComponent_IsAwake(Entity.ID); } }
			set { unsafe { InternalCalls.RigidBody2DComponent_SetAwake(Entity.ID, value); } }
		}

		public bool Enabled
		{
			get { unsafe { return InternalCalls.RigidBody2DComponent_IsEnabled(Entity.ID); } }
			set { unsafe { InternalCalls.RigidBody2DComponent_SetEnabled(Entity.ID, value); } }
		}

		public bool FixedRotation
		{
			get { unsafe { return InternalCalls.RigidBody2DComponent_IsFixedRotation(Entity.ID); } }
			set { unsafe { InternalCalls.RigidBody2DComponent_SetFixedRotation(Entity.ID, value); } }
		}

		public void ApplyForce(Vector2 force, Vector2 point, PhysicsForce2DType forceType = PhysicsForce2DType.Force)
		{
			unsafe { InternalCalls.RigidBody2DComponent_ApplyForce(Entity.ID, &force, &point, forceType); }
		}

		public void ApplyForce(Vector2 force, PhysicsForce2DType forceType = PhysicsForce2DType.Force)
		{
			unsafe { InternalCalls.RigidBody2DComponent_ApplyForceToCenter(Entity.ID, &force, forceType); }
		}

		public void ApplyTorque(float torque, PhysicsForce2DType forceType = PhysicsForce2DType.Force)
		{
			unsafe { InternalCalls.RigidBody2DComponent_ApplyTorque(Entity.ID, torque, forceType); }
		}
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
			get { unsafe { return InternalCalls.BoxCollider2DComponent_IsSensor(Entity.ID); } }
			set { unsafe { InternalCalls.BoxCollider2DComponent_SetSensor(Entity.ID, value); } }
		}

		public override float Density
		{
			get { unsafe { return InternalCalls.BoxCollider2DComponent_GetDensity(Entity.ID); } }
			set { unsafe { InternalCalls.BoxCollider2DComponent_SetDensity(Entity.ID, value); } }
		}

		public override float Friction
		{
			get { unsafe { return InternalCalls.BoxCollider2DComponent_GetFriction(Entity.ID); } }
			set { unsafe { InternalCalls.BoxCollider2DComponent_SetFriction(Entity.ID, value); } }
		}

		public override float Restitution
		{
			get { unsafe { return InternalCalls.BoxCollider2DComponent_GetRestitution(Entity.ID); } }
			set { unsafe { InternalCalls.BoxCollider2DComponent_SetRestitution(Entity.ID, value); } }
		}

		public override float RestitutionThreshold
		{
			get { unsafe { return InternalCalls.BoxCollider2DComponent_GetRestitutionThreshold(Entity.ID); } }
			set { unsafe { InternalCalls.BoxCollider2DComponent_SetRestitutionThreshold(Entity.ID, value); } }
		}

		public Vector2 Size
		{
			get
			{
				Vector2 result = new();
				unsafe { InternalCalls.BoxCollider2DComponent_GetSize(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe { InternalCalls.BoxCollider2DComponent_SetSize(Entity.ID, &value); }
			}
		}

		public Vector2 Offset
		{
			get
			{
				Vector2 result = new();
				unsafe { InternalCalls.BoxCollider2DComponent_GetOffset(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe { InternalCalls.BoxCollider2DComponent_SetOffset(Entity.ID, &value); }
			}
		}

		public float Rotation
		{
			get { unsafe { return InternalCalls.BoxCollider2DComponent_GetRotation(Entity.ID); } }
			set { unsafe { InternalCalls.BoxCollider2DComponent_SetRotation(Entity.ID, value); } }
		}
	}

	public class CircleCollider2DComponent : Collider2D
	{
		public override bool Sensor
		{
			get { unsafe { return InternalCalls.CircleCollider2DComponent_IsSensor(Entity.ID); } }
			set { unsafe { InternalCalls.CircleCollider2DComponent_SetSensor(Entity.ID, value); } }
		}

		public override float Density
		{
			get { unsafe { return InternalCalls.CircleCollider2DComponent_GetDensity(Entity.ID); } }
			set { unsafe { InternalCalls.CircleCollider2DComponent_SetDensity(Entity.ID, value); } }
		}

		public override float Friction
		{
			get { unsafe { return InternalCalls.CircleCollider2DComponent_GetFriction(Entity.ID); } }
			set { unsafe { InternalCalls.CircleCollider2DComponent_SetFriction(Entity.ID, value); } }
		}

		public override float Restitution
		{
			get { unsafe { return InternalCalls.CircleCollider2DComponent_GetRestitution(Entity.ID); } }
			set { unsafe { InternalCalls.CircleCollider2DComponent_SetRestitution(Entity.ID, value); } }
		}

		public override float RestitutionThreshold
		{
			get { unsafe { return InternalCalls.CircleCollider2DComponent_GetRestitutionThreshold(Entity.ID); } }
			set { unsafe { InternalCalls.CircleCollider2DComponent_SetRestitutionThreshold(Entity.ID, value); } }
		}

		public float Radius
		{
			get { unsafe { return InternalCalls.CircleCollider2DComponent_GetRadius(Entity.ID); } }
			set { unsafe { InternalCalls.CircleCollider2DComponent_SetRadius(Entity.ID, value); } }
		}

		public Vector2 Offset
		{
			get
			{
				Vector2 result = new();
				unsafe { InternalCalls.CircleCollider2DComponent_GetOffset(Entity.ID, &result); return result; }
			}
			set
			{
				unsafe { InternalCalls.CircleCollider2DComponent_SetOffset(Entity.ID, value); }
			}
		}

		public float Rotation
		{
			get { unsafe { return InternalCalls.CircleCollider2DComponent_GetRotation(Entity.ID); } }
			set { unsafe { InternalCalls.CircleCollider2DComponent_SetRotation(Entity.ID, value); } }
		}
	}

	public class ScriptComponent : Component
	{
		public NativeInstance<object> Instance
		{
			get
			{
				unsafe { return InternalCalls.ScriptComponent_GetInstance(Entity.ID); }
			}
		}
	}

}
