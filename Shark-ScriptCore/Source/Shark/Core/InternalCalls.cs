using Coral.Managed.Interop;
using Shark;

namespace Shark
{
	internal static unsafe class InternalCalls
	{

#pragma warning disable CS0649

		#region AssetHandle

		internal static unsafe delegate* unmanaged<AssetHandle, bool> AssetHandle_IsValid;

		#endregion

		#region Application

		internal static unsafe delegate* unmanaged<uint> Application_GetWidth;
		internal static unsafe delegate* unmanaged<uint> Application_GetHeight;

		#endregion

		#region Log

		internal static unsafe delegate* unmanaged<Log.Level, NativeString, void> Log_LogMessage;

		#endregion

		#region Input

		internal static unsafe delegate* unmanaged<KeyCode, KeyState, bool> Input_IsKeyStateSet;
		internal static unsafe delegate* unmanaged<MouseButton, MouseState, bool> Input_IsMouseStateSet;
		internal static unsafe delegate* unmanaged<float> Input_GetMouseScroll;
		internal static unsafe delegate* unmanaged<Vector2i*, void> Input_GetMousePos;
		
		#endregion

		#region Matrix4

		internal static unsafe delegate* unmanaged<Matrix4*, Matrix4*, void> Matrix4_Inverse;
		internal static unsafe delegate* unmanaged<Matrix4*, Matrix4*, Matrix4*, void> Matrix4_Matrix4MulMatrix4;
		internal static unsafe delegate* unmanaged<Matrix4*, Vector4*, Vector4*, void> Matrix4_Matrix4MulVector4;

		#endregion

		#region Scene

		internal static unsafe delegate* unmanaged<ulong, bool> Scene_IsEntityValid;
		internal static unsafe delegate* unmanaged<NativeString, ulong> Scene_CreateEntity;
		internal static unsafe delegate* unmanaged<ulong, void> Scene_DestroyEntity;
		internal static unsafe delegate* unmanaged<NativeString, ulong> Scene_FindEntityByTag;

		#endregion

		#region Entity

		internal static unsafe delegate* unmanaged<ulong, ulong> Entity_GetParent;
		internal static unsafe delegate* unmanaged<ulong, ulong, void> Entity_SetParent;
		internal static unsafe delegate* unmanaged<ulong, NativeArray<ulong>> Entity_GetChildren;

		internal static unsafe delegate* unmanaged<ulong, ReflectionType, bool> Entity_HasComponent;
		internal static unsafe delegate* unmanaged<ulong, ReflectionType, void> Entity_AddComponent;
		internal static unsafe delegate* unmanaged<ulong, ReflectionType, void> Entity_RemoveComponent;

		#endregion

		#region TagComponent

		internal static unsafe delegate* unmanaged<ulong, string> TagComponent_GetTag;
		internal static unsafe delegate* unmanaged<ulong, NativeString, void> TagComponent_SetTag;

		#endregion

		#region TransformComponent

		internal static unsafe delegate* unmanaged<ulong, Vector3*, void> TransformComponent_GetTranslation;
		internal static unsafe delegate* unmanaged<ulong, Vector3*, void> TransformComponent_SetTranslation;
		internal static unsafe delegate* unmanaged<ulong, Vector3*, void> TransformComponent_GetRotation;
		internal static unsafe delegate* unmanaged<ulong, Vector3*, void> TransformComponent_SetRotation;
		internal static unsafe delegate* unmanaged<ulong, Vector3*, void> TransformComponent_GetScale;
		internal static unsafe delegate* unmanaged<ulong, Vector3*, void> TransformComponent_SetScale;
		internal static unsafe delegate* unmanaged<ulong, Transform*, void> TransformComponent_GetLocalTransform;
		internal static unsafe delegate* unmanaged<ulong, Transform*, void> TransformComponent_SetLocalTransform;
		internal static unsafe delegate* unmanaged<ulong, Transform*, void> TransformComponent_GetWorldTransform;
		internal static unsafe delegate* unmanaged<ulong, Transform*, void> TransformComponent_SetWorldTransform;

		#endregion

		#region SpriteRendererComponent

		internal static unsafe delegate* unmanaged<ulong, Color*, void> SpriteRendererComponent_GetColor;
		internal static unsafe delegate* unmanaged<ulong, Color*, void> SpriteRendererComponent_SetColor;
		internal static unsafe delegate* unmanaged<ulong, AssetHandle> SpriteRendererComponent_GetTextureHandle;
		internal static unsafe delegate* unmanaged<ulong, AssetHandle, void> SpriteRendererComponent_SetTextureHandle;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, void> SpriteRendererComponent_GetTilingFactor;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, void> SpriteRendererComponent_SetTilingFactor;

		#endregion

		#region CricleRendererComponent

		internal static unsafe delegate* unmanaged<ulong, Color*, void> CircleRendererComponent_GetColor;
		internal static unsafe delegate* unmanaged<ulong, Color*, void> CircleRendererComponent_SetColor;
		internal static unsafe delegate* unmanaged<ulong, float> CircleRendererComponent_GetThickness;
		internal static unsafe delegate* unmanaged<ulong, float, void> CircleRendererComponent_SetThickness;
		internal static unsafe delegate* unmanaged<ulong, float> CircleRendererComponent_GetFade;
		internal static unsafe delegate* unmanaged<ulong, float, void> CircleRendererComponent_SetFade;

		#endregion

		#region CameraComponent

		internal static unsafe delegate* unmanaged<ulong, Matrix4*, void> CameraComponent_GetProjection;
		internal static unsafe delegate* unmanaged<ulong, Matrix4*, void> CameraComponent_SetProjection;
		internal static unsafe delegate* unmanaged<ulong, bool> CameraComponent_GetIsPerspective;
		internal static unsafe delegate* unmanaged<ulong, bool, void> CameraComponent_SetIsPerspective;
		internal static unsafe delegate* unmanaged<ulong, float, float, float, float, void> CameraComponent_SetPerspective;
		internal static unsafe delegate* unmanaged<ulong, float, float, float, float, void> CameraComponent_SetOrthographic;
		internal static unsafe delegate* unmanaged<ulong, float> CameraComponent_GetAspectratio;
		internal static unsafe delegate* unmanaged<ulong, float, void> CameraComponent_SetAspectratio;
		internal static unsafe delegate* unmanaged<ulong, float> CameraComponent_GetPerspectiveFOV;
		internal static unsafe delegate* unmanaged<ulong, float, void> CameraComponent_SetPerspectiveFOV;
		internal static unsafe delegate* unmanaged<ulong, float> CameraComponent_GetPerspectiveNear;
		internal static unsafe delegate* unmanaged<ulong, float, void> CameraComponent_SetPerspectiveNear;
		internal static unsafe delegate* unmanaged<ulong, float> CameraComponent_GetPerspectiveFar;
		internal static unsafe delegate* unmanaged<ulong, float, void> CameraComponent_SetPerspectiveFar;
		internal static unsafe delegate* unmanaged<ulong, float> CameraComponent_GetOrthographicZoom;
		internal static unsafe delegate* unmanaged<ulong, float, void> CameraComponent_SetOrthographicZoom;
		internal static unsafe delegate* unmanaged<ulong, float> CameraComponent_GetOrthographicNear;
		internal static unsafe delegate* unmanaged<ulong, float, void> CameraComponent_SetOrthographicNear;
		internal static unsafe delegate* unmanaged<ulong, float> CameraComponent_GetOrthographicFar;
		internal static unsafe delegate* unmanaged<ulong, float, void> CameraComponent_SetOrthographicFar;

		#endregion

		#region Physics2D

		internal static unsafe delegate* unmanaged<Vector2*, void> Physics2D_GetGravity;
		internal static unsafe delegate* unmanaged<Vector2*, void> Physics2D_SetGravity;
		internal static unsafe delegate* unmanaged<bool> Physics2D_GetAllowSleep;
		internal static unsafe delegate* unmanaged<bool, bool> Physics2D_SetAllowSleep;

		#endregion

		#region RigidBody2DComponent

		internal static unsafe delegate* unmanaged<ulong, RigidBody2DType> RigidBody2DComponent_GetBodyType;
		internal static unsafe delegate* unmanaged<ulong, RigidBody2DType, void> RigidBody2DComponent_SetBodyType;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, void> RigidBody2DComponent_GetPosition;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, void> RigidBody2DComponent_SetPosition;
		internal static unsafe delegate* unmanaged<ulong, float> RigidBody2DComponent_GetRotation;
		internal static unsafe delegate* unmanaged<ulong, float, void> RigidBody2DComponent_SetRotation;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, void> RigidBody2DComponent_GetLocalCenter;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, void> RigidBody2DComponent_GetWorldCenter;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, void> RigidBody2DComponent_GetLinearVelocity;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, void> RigidBody2DComponent_SetLinearVelocity;
		internal static unsafe delegate* unmanaged<ulong, float> RigidBody2DComponent_GetAngularVelocity;
		internal static unsafe delegate* unmanaged<ulong, float, void> RigidBody2DComponent_SetAngularVelocity;
		internal static unsafe delegate* unmanaged<ulong, float> RigidBody2DComponent_GetGravityScale;
		internal static unsafe delegate* unmanaged<ulong, float, void> RigidBody2DComponent_SetGravityScale;
		internal static unsafe delegate* unmanaged<ulong, float> RigidBody2DComponent_GetLinearDamping;
		internal static unsafe delegate* unmanaged<ulong, float, void> RigidBody2DComponent_SetLinearDamping;
		internal static unsafe delegate* unmanaged<ulong, float> RigidBody2DComponent_GetAngularDamping;
		internal static unsafe delegate* unmanaged<ulong, float, void> RigidBody2DComponent_SetAngularDamping;
		internal static unsafe delegate* unmanaged<ulong, bool> RigidBody2DComponent_IsBullet;
		internal static unsafe delegate* unmanaged<ulong, bool, void> RigidBody2DComponent_SetBullet;
		internal static unsafe delegate* unmanaged<ulong, bool> RigidBody2DComponent_IsSleepingAllowed;
		internal static unsafe delegate* unmanaged<ulong, bool, void> RigidBody2DComponent_SetSleepingAllowed;
		internal static unsafe delegate* unmanaged<ulong, bool> RigidBody2DComponent_IsAwake;
		internal static unsafe delegate* unmanaged<ulong, bool, void> RigidBody2DComponent_SetAwake;
		internal static unsafe delegate* unmanaged<ulong, bool> RigidBody2DComponent_IsEnabled;
		internal static unsafe delegate* unmanaged<ulong, bool, void> RigidBody2DComponent_SetEnabled;
		internal static unsafe delegate* unmanaged<ulong, bool> RigidBody2DComponent_IsFixedRotation;
		internal static unsafe delegate* unmanaged<ulong, bool, void> RigidBody2DComponent_SetFixedRotation;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, Vector2*, PhysicsForce2DType, void> RigidBody2DComponent_ApplyForce;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, PhysicsForce2DType, void> RigidBody2DComponent_ApplyForceToCenter;
		internal static unsafe delegate* unmanaged<ulong, float, PhysicsForce2DType, void> RigidBody2DComponent_ApplyTorque;

		#endregion

		#region BoxCollider2DComponent

		internal static unsafe delegate* unmanaged<ulong, bool> BoxCollider2DComponent_IsSensor;
		internal static unsafe delegate* unmanaged<ulong, bool, void> BoxCollider2DComponent_SetSensor;
		internal static unsafe delegate* unmanaged<ulong, float> BoxCollider2DComponent_GetDensity;
		internal static unsafe delegate* unmanaged<ulong, float, void> BoxCollider2DComponent_SetDensity;
		internal static unsafe delegate* unmanaged<ulong, float> BoxCollider2DComponent_GetFriction;
		internal static unsafe delegate* unmanaged<ulong, float, void> BoxCollider2DComponent_SetFriction;
		internal static unsafe delegate* unmanaged<ulong, float> BoxCollider2DComponent_GetRestitution;
		internal static unsafe delegate* unmanaged<ulong, float, void> BoxCollider2DComponent_SetRestitution;
		internal static unsafe delegate* unmanaged<ulong, float> BoxCollider2DComponent_GetRestitutionThreshold;
		internal static unsafe delegate* unmanaged<ulong, float, void> BoxCollider2DComponent_SetRestitutionThreshold;

		internal static unsafe delegate* unmanaged<ulong, Vector2*, void> BoxCollider2DComponent_GetSize;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, void> BoxCollider2DComponent_SetSize;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, void> BoxCollider2DComponent_GetOffset;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, void> BoxCollider2DComponent_SetOffset;
		internal static unsafe delegate* unmanaged<ulong, float> BoxCollider2DComponent_GetRotation;
		internal static unsafe delegate* unmanaged<ulong, float, void> BoxCollider2DComponent_SetRotation;

		#endregion

		#region CircleCollider2DComponent

		internal static unsafe delegate* unmanaged<ulong, bool> CircleCollider2DComponent_IsSensor;
		internal static unsafe delegate* unmanaged<ulong, bool, void> CircleCollider2DComponent_SetSensor;
		internal static unsafe delegate* unmanaged<ulong, float> CircleCollider2DComponent_GetDensity;
		internal static unsafe delegate* unmanaged<ulong, float, void> CircleCollider2DComponent_SetDensity;
		internal static unsafe delegate* unmanaged<ulong, float> CircleCollider2DComponent_GetFriction;
		internal static unsafe delegate* unmanaged<ulong, float, void> CircleCollider2DComponent_SetFriction;
		internal static unsafe delegate* unmanaged<ulong, float> CircleCollider2DComponent_GetRestitution;
		internal static unsafe delegate* unmanaged<ulong, float, void> CircleCollider2DComponent_SetRestitution;
		internal static unsafe delegate* unmanaged<ulong, float> CircleCollider2DComponent_GetRestitutionThreshold;
		internal static unsafe delegate* unmanaged<ulong, float, void> CircleCollider2DComponent_SetRestitutionThreshold;

		internal static unsafe delegate* unmanaged<ulong, float> CircleCollider2DComponent_GetRadius;
		internal static unsafe delegate* unmanaged<ulong, float, void> CircleCollider2DComponent_SetRadius;
		internal static unsafe delegate* unmanaged<ulong, Vector2*, void> CircleCollider2DComponent_GetOffset;
		internal static unsafe delegate* unmanaged<ulong, Vector2, void> CircleCollider2DComponent_SetOffset;
		internal static unsafe delegate* unmanaged<ulong, float> CircleCollider2DComponent_GetRotation;
		internal static unsafe delegate* unmanaged<ulong, float, void> CircleCollider2DComponent_SetRotation;

		#endregion

		#region ScriptComponent

		internal static unsafe delegate* unmanaged<ulong, NativeInstance<object>> ScriptComponent_GetInstance;

		#endregion

#pragma warning restore CS0649

	}

}
