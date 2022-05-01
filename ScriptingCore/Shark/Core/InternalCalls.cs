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
		internal static extern void Matrix4_Inverse(ref Matrix4 matrix, ref Matrix4 out_Result);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Matrix4 Matrix4_Matrix4MulMatrix4(ref Matrix4 lhs, ref Matrix4 rhs);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Vector4 Matrix4_Matrix4MulVector4(ref Matrix4 lhs, ref Vector4 rhs);


		//[MethodImpl(MethodImplOptions.InternalCall)]
		//internal static extern Vector4 Matrix4_Translate(ref Matrix4 mat, ref Vector4 translation);
		
		//[MethodImpl(MethodImplOptions.InternalCall)]
		//internal static extern Vector4 Matrix4_Rotate(ref Matrix4 mat, ref Vector4 rotation);
		
		//[MethodImpl(MethodImplOptions.InternalCall)]
		//internal static extern Vector4 Matrix4_Scale(ref Matrix4 mat, ref Vector4 scaling);
		
		//[MethodImpl(MethodImplOptions.InternalCall)]
		//internal static extern Vector4 Matrix4_Transform(ref Matrix4 mat, ref Transform transform);


		#endregion

		#region Scene

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern object Scene_InstantiateScript(string name, System.Type scriptType);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Scene_CreateEntity(string name, UUID uuid, ref UUID out_UUID);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Scene_DestroyEntity(UUID entityHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Entity Scene_GetScriptObject(UUID scriptEntityHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Scene_IsValidEntityHandle(UUID entityHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern UUID Scene_GetActiveCameraUUID();

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Scene_GetUUIDFromTag(string tag, ref UUID out_UUID);

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
		internal static extern RigidBody2DTransform RigidBody2DComponent_GetTransform(System.IntPtr nativeHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetTransform(System.IntPtr nativeHandle, ref RigidBody2DTransform transform);
		
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
		internal static extern void RigidBody2DComponent_ApplyForce(System.IntPtr nativeHandle, ref Vector2 force, ref Vector2 point, bool wake);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyForceToCenter(System.IntPtr nativeHandle, ref Vector2 force, bool wake);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyTorque(System.IntPtr nativeHandle, float torque, bool wake);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyLinearImpulse(System.IntPtr nativeHandle, ref Vector2 impulse, ref Vector2 point, bool wake);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyLinearImpulseToCenter(System.IntPtr nativeHandle, ref Vector2 impulse, bool wake);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyAngularImpulse(System.IntPtr nativeHandle, float impulse, bool wake);

		#endregion

	}

}
