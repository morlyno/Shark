using System.Runtime.CompilerServices;

namespace Shark
{
	internal static class InternalCalls
	{

		#region Log

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Log_LogLevel(Log.Level level, string msg);

		#endregion

		#region UUID

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern UUID UUID_Generate();

		#endregion

		#region Input

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_KeyPressed(Key key);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_MouseButtonPressed(Mouse button);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector2i Input_GetMousePos();
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector2i Input_GetMousePosTotal();

		#endregion

		#region Matrix4

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Matrix4_Inverse(ref Matrix4 matrix, out Matrix4 out_Result);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Matrix4 Matrix4_Matrix4MulMatrix4(ref Matrix4 lhs, ref Matrix4 rhs);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector4 Matrix4_Matrix4MulVector4(ref Matrix4 lhs, ref Vector4 rhs);

		#endregion

		#region Scene

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern object Scene_InstantiateScript(System.Type scriptType, string name);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Scene_CreateEntity(string name, UUID uuid, out UUID out_UUID);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Scene_DestroyEntity(UUID entityHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Scene_CloneEntity(UUID entityHandle, out UUID out_UUID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Entity Scene_GetScriptObject(UUID scriptEntityHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Scene_IsValidEntityHandle(UUID entityHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern UUID Scene_GetActiveCameraUUID();

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Scene_GetUUIDFromTag(string tag, out UUID out_UUID);

		#endregion

		#region Entity


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern string Entity_GetName(UUID entityHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Entity_SetName(UUID entityHandle, string name);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Entity_HasComponent(UUID entityHandle, System.Type compType);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Entity_AddComponent(UUID entityHandle, System.Type compType);

		#endregion

		#region TransformComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector3 TransformComponent_GetTranslation(UUID owner);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetTranslation(UUID owner, Vector3 translation);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector3 TransformComponent_GetRotation(UUID owner);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetRotation(UUID owner, Vector3 rotation);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector3 TransformComponent_GetScaling(UUID owner);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetScaling(UUID owner, Vector3 scaling);

		#endregion

		#region SpriteRendererComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Color SpriteRendererComponent_GetColor(UUID entityHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_SetColor(UUID entityHandle, Color color);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern UUID SpriteRendererComponent_GetTextureHandle(UUID entityHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_SetTextureHandle(UUID entityHandle, UUID textureHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SpriteRendererComponent_GetTilingFactor(UUID entityHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_SetTilingFactor(UUID entityHandle, float tilingFactor);

		#endregion

		#region CricleRendererComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Color CircleRendererComponent_GetColor(UUID entityHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleRendererComponent_SetColor(UUID entityHandle, Color color);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CircleRendererComponent_GetThickness(UUID entityHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleRendererComponent_SetThickness(UUID entityHandle, float thickness);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CircleRendererComponent_GetFade(UUID entityHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleRendererComponent_SetFade(UUID entityHandle, float fade);

		#endregion

		#region CameraComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Matrix4 CameraComponent_GetProjection(UUID entityHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetProjection(UUID entityHandle, Matrix4 projection);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern CameraComponent.ProjectionType CameraComponent_GetProjectionType(UUID entityHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetProjectionType(UUID entityHandle, CameraComponent.ProjectionType projectionType);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspective(UUID entityHandle, float aspectratio, float fov, float clipnear, float clipfar);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographic(UUID entityHandle, float aspectratio, float zoom, float clipnear, float clipfar);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetAspectratio(UUID entityHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetAspectratio(UUID entityHandle, float aspectratio);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspectiveFOV(UUID entityHandle, float fov);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetPerspectiveFOV(UUID entityHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspectiveNear(UUID entityHandle, float near);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetPerspectiveNear(UUID entityHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspectiveFar(UUID entityHandle, float far);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetPerspectiveFar(UUID entityHandle);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographicZoom(UUID entityHandle, float zoom);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetOrthographicZoom(UUID entityHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographicNear(UUID entityHandle, float near);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetOrthographicNear(UUID entityHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographicFar(UUID entityHandle, float far);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetOrthographicFar(UUID entityHandle);

		#endregion

		#region RigidBody2DComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern System.IntPtr RigidBody2DComponent_GetNativeHandle(UUID owner);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern RigidBody2DType RigidBody2DComponent_GetBodyType(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetBodyType(System.IntPtr nativeHandle, RigidBody2DType bodyType);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern RigidBody2DTransform RigidBody2DComponent_GetTransform(System.IntPtr nativeHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetTransform(System.IntPtr nativeHandle, ref RigidBody2DTransform transform);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetPosition(System.IntPtr nativeHandle, Vector2 position);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetRotation(System.IntPtr nativeHandle, float rotation);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector2 RigidBody2DComponent_GetLocalCenter(System.IntPtr nativeHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector2 RigidBody2DComponent_GetWorldCenter(System.IntPtr nativeHandle);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector2 RigidBody2DComponent_GetLinearVelocity(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetLinearVelocity(System.IntPtr nativeHandle, ref Vector2 linearVelocity);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBody2DComponent_GetAngularVelocity(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetAngularVelocity(System.IntPtr nativeHandle, float angularVelocity);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBody2DComponent_GetGravityScale(System.IntPtr nativeHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetGravityScale(System.IntPtr nativeHandle, float gravityScale);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBody2DComponent_GetLinearDamping(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetLinearDamping(System.IntPtr nativeHandle, float linearDamping);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBody2DComponent_GetAngularDamping(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetAngularDamping(System.IntPtr nativeHandle, float angularDamping);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsBullet(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetBullet(System.IntPtr nativeHandle, bool bullet);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsSleepingAllowed(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetSleepingAllowed(System.IntPtr nativeHandle, bool sleepingAllowed);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsAwake(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetAwake(System.IntPtr nativeHandle, bool awake);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsEnabled(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetEnabled(System.IntPtr nativeHandle, bool enabled);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsFixedRotation(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetFixedRotation(System.IntPtr nativeHandle, bool fixedRotation);



		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyForce(System.IntPtr nativeHandle, ref Vector2 force, ref Vector2 point, PhysicsForce2DType forceType);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyForceToCenter(System.IntPtr nativeHandle, ref Vector2 force, PhysicsForce2DType forceType);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyTorque(System.IntPtr nativeHandle, float torque, PhysicsForce2DType forceType);

		#endregion

		#region PhysicsCollider2D

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void PhysicsCollider2D_SetSensor(System.IntPtr nativeHandle, bool sensor);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool PhysicsCollider2D_IsSensor(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void PhysicsCollider2D_SetDensity(System.IntPtr nativeHandle, float density);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float PhysicsCollider2D_GetDensity(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void PhysicsCollider2D_SetFriction(System.IntPtr nativeHandle, float friction);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float PhysicsCollider2D_GetFriction(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void PhysicsCollider2D_SetRestitution(System.IntPtr nativeHandle, float restitution);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float PhysicsCollider2D_GetRestitution(System.IntPtr nativeHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void PhysicsCollider2D_SetRestitutionThreshold(System.IntPtr nativeHandle, float restitutionThreshold);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float PhysicsCollider2D_GetRestitutionThreshold(System.IntPtr nativeHandle);

		#endregion

		#region BoxCollider2DComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern System.IntPtr BoxCollider2DComponent_GetNativeHandle(UUID owner);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector2 BoxCollider2DComponent_GetSize(UUID owner);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxCollider2DComponent_SetSize(UUID owner, Vector2 size);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector2 BoxCollider2DComponent_GetOffset(UUID owner);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxCollider2DComponent_SetOffset(UUID owner, Vector2 offset);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float BoxCollider2DComponent_GetRotation(UUID owner);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxCollider2DComponent_SetRotation(UUID owner, float rotation);

		#endregion

		#region CircleCollider2DComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern System.IntPtr CircleCollider2DComponent_GetNativeHandle(UUID owner);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CircleCollider2DComponent_GetRadius(UUID owner);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleCollider2DComponent_SetRadius(UUID owner, float Radius);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector2 CircleCollider2DComponent_GetOffset(UUID owner);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleCollider2DComponent_SetOffset(UUID owner, Vector2 offset);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CircleCollider2DComponent_GetRotation(UUID owner);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleCollider2DComponent_SetRotation(UUID owner, float rotation);

		#endregion

	}

}
