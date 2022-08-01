using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace Shark
{
	internal static class InternalCalls
	{

		#region Application

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern uint Application_GetWidth();

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern uint Application_GetHeight();

		#endregion

		#region Log

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Log_LogLevel(Log.Level level, string msg);

		#endregion

		#region Input

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_KeyPressed(Key key);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_MouseButtonPressed(MouseButton button);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_GetMousePos(out Vector2i mousePos);
		
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
		internal static extern void Scene_CreateEntity(string name, ulong id, out ulong out_ID);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Scene_DestroyEntity(ulong entityHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Scene_CloneEntity(ulong entityHandle, out ulong out_ID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Entity Scene_GetScriptObject(ulong scriptEntityHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Scene_IsValidEntityHandle(ulong id);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Scene_GetActiveCameraUUID(out ulong out_ID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Scene_GetUUIDFromTag(string tag, out ulong out_ID);

		#endregion

		#region Entity

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Entity_HasComponent(ulong id, System.Type compType);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Entity_AddComponent(ulong id, System.Type compType);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Entity_RemoveComponent(ulong id, System.Type compType);

		#endregion

		#region TagComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool TagComponent_GetTag(ulong id, out string tag);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool TagComponent_SetTag(ulong id, string tag);

		#endregion

		#region TransformComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool TransformComponent_GetTranslation(ulong id, out Vector3 translation);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool TransformComponent_SetTranslation(ulong id, ref Vector3 translation);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool TransformComponent_GetRotation(ulong id, out Vector3 rotation);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool TransformComponent_SetRotation(ulong id, ref Vector3 rotation);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool TransformComponent_GetScale(ulong id, out Vector3 scale);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool TransformComponent_SetScale(ulong id, ref Vector3 scale);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool TransformComponent_GetLocalTransform(ulong id, out Transform out_LocalTransform);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool TransformComponent_SetLocalTransform(ulong id, ref Transform localTransform);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool TransformComponent_GetWorldTransform(ulong id, out Transform out_WorldTransform);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool TransformComponent_SetWorldTransform(ulong id, ref Transform worldTransform);

		#endregion

		#region SpriteRendererComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool SpriteRendererComponent_GetColor(ulong id, out Color color);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool SpriteRendererComponent_SetColor(ulong id, ref Color color);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool SpriteRendererComponent_GetTextureHandle(ulong id, out AssetHandle assetHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool SpriteRendererComponent_SetTextureHandle(ulong id, ref AssetHandle textureHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool SpriteRendererComponent_GetTilingFactor(ulong id, out float tilingFactor);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool SpriteRendererComponent_SetTilingFactor(ulong id, float tilingFactor);

		#endregion

		#region CricleRendererComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleRendererComponent_GetColor(ulong id, out Color color);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleRendererComponent_SetColor(ulong id, ref Color color);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleRendererComponent_GetThickness(ulong id, out float thickness);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleRendererComponent_SetThickness(ulong id, float thickness);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleRendererComponent_GetFade(ulong id, out float fade);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleRendererComponent_SetFade(ulong id, float fade);

		#endregion

		#region CameraComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_GetProjection(ulong id, out Matrix4 projection);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_SetProjection(ulong id, ref Matrix4 projection);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_GetProjectionType(ulong id, out CameraComponent.ProjectionType projectionType);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_SetProjectionType(ulong id, CameraComponent.ProjectionType projectionType);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspective(ulong id, float aspectratio, float fov, float clipnear, float clipfar);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographic(ulong id, float aspectratio, float zoom, float clipnear, float clipfar);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_GetAspectratio(ulong id, out float aspectratio);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_SetAspectratio(ulong id, float aspectratio);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_SetPerspectiveFOV(ulong id, float fov);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_GetPerspectiveFOV(ulong id, out float fov);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_SetPerspectiveNear(ulong id, float near);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_GetPerspectiveNear(ulong id, out float near);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_SetPerspectiveFar(ulong id, float far);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_GetPerspectiveFar(ulong id, out float far);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_SetOrthographicZoom(ulong id, float zoom);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_GetOrthographicZoom(ulong id, out float zoom);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_SetOrthographicNear(ulong id, float near);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_GetOrthographicNear(ulong id, out float near);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_SetOrthographicFar(ulong id, float far);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_GetOrthographicFar(ulong id, out float far);

		#endregion

		#region RigidBody2DComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_GetBodyType(ulong id, out RigidBody2DType bodyType);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetBodyType(ulong id, RigidBody2DType bodyType);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_GetTransform(ulong id, out RigidBody2DTransform transform);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetTransform(ulong id, ref RigidBody2DTransform transform);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetPosition(ulong id, ref Vector2 position);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetRotation(ulong id, float rotation);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_GetLocalCenter(ulong id, out Vector2 localCenter);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_GetWorldCenter(ulong id, out Vector2 worldCenter);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_GetLinearVelocity(ulong id, out Vector2 linearVelocity);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetLinearVelocity(ulong id, ref Vector2 linearVelocity);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_GetAngularVelocity(ulong id, out float angularVelocity);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetAngularVelocity(ulong id, float angularVelocity);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_GetGravityScale(ulong id, out float gravityScale);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetGravityScale(ulong id, float gravityScale);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_GetLinearDamping(ulong id, out float linearDamping);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetLinearDamping(ulong id, float linearDamping);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_GetAngularDamping(ulong id, out float angularDamping);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetAngularDamping(ulong id, float angularDamping);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsBullet(ulong id, out bool IsBullet);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetBullet(ulong id, bool bullet);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsSleepingAllowed(ulong id, out bool sleepingAllowed);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetSleepingAllowed(ulong id, bool sleepingAllowed);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsAwake(ulong id, out bool isAwake);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetAwake(ulong id, bool awake);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsEnabled(ulong id, out bool isEnabled);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetEnabled(ulong id, bool enabled);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsFixedRotation(ulong id, out bool fixedRotation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_SetFixedRotation(ulong id, bool fixedRotation);



		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyForce(ulong id, ref Vector2 force, ref Vector2 point, PhysicsForce2DType forceType);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyForceToCenter(ulong id, ref Vector2 force, PhysicsForce2DType forceType);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyTorque(ulong id, float torque, PhysicsForce2DType forceType);

		#endregion

		#region BoxCollider2DComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_SetSensor(ulong id, bool sensor);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_IsSensor(ulong id, out bool isSensor);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_SetDensity(ulong id, float density);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_GetDensity(ulong id, out float density);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_SetFriction(ulong id, float friction);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_GetFriction(ulong id, out float friction);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_SetRestitution(ulong id, float restitution);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_GetRestitution(ulong id, out float restitution);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_SetRestitutionThreshold(ulong id, float restitutionThreshold);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_GetRestitutionThreshold(ulong id, out float restitutionThreshold);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_GetSize(ulong id, out Vector2 size);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_SetSize(ulong id, ref Vector2 size);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_GetOffset(ulong id, out Vector2 offset);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_SetOffset(ulong id, ref Vector2 offset);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_GetRotation(ulong id, out float rotation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_SetRotation(ulong id, float rotation);

		#endregion

		#region CircleCollider2DComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_SetSensor(ulong id, bool sensor);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_IsSensor(ulong id, out bool isSensor);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_SetDensity(ulong id, float density);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_GetDensity(ulong id, out float density);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_SetFriction(ulong id, float friction);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_GetFriction(ulong id, out float friction);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_SetRestitution(ulong id, float restitution);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_GetRestitution(ulong id, out float restitution);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_SetRestitutionThreshold(ulong id, float restitutionThreshold);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_GetRestitutionThreshold(ulong id, out float restitutionThreshold);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_GetRadius(ulong id, out float radius);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_SetRadius(ulong id, float Radius);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_GetOffset(ulong id, out Vector2 offset);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_SetOffset(ulong id, Vector2 offset);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_GetRotation(ulong id, out float rotation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_SetRotation(ulong id, float rotation);

		#endregion

		#region ResourceManager

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool ResourceManager_GetAssetHandleFromFilePath(string filePath, out AssetHandle assetHandle);

		#endregion

		#region EditorUI

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool EditorUI_BeginWindow(string windowTitle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void EditorUI_EndWindow();

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void EditorUI_Text(string text);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void EditorUI_NewLine();
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void EditorUI_Separator();


		#endregion

	}

}
