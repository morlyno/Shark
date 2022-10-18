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
		internal static extern void Log_LogMessage(Log.Level level, string msg);

		#endregion

		#region Input

		internal enum KeyState : ushort
		{
			None = 0,
			Pressed,
			Down,
			Released
		}

		internal enum MouseState : ushort
		{
			None = 0,
			Pressed,
			Down,
			Released,
			DoubleClicked
		}

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsKeyStateSet(KeyCode key, KeyState keyState);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsMouseStateSet(MouseButton key, MouseState mouseState);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float Input_GetMouseScroll();

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Input_GetMousePos(out Vector2i mousePos);
		
		#endregion

		#region Matrix4

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Matrix4_Inverse(ref Matrix4 matrix, out Matrix4 out_Result);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Matrix4_Matrix4MulMatrix4(ref Matrix4 lhs, ref Matrix4 rhs, out Matrix4 result);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Matrix4_Matrix4MulVector4(ref Matrix4 lhs, ref Vector4 rhs, out Vector4 result);

		#endregion

		#region Scene

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern object Scene_Instantiate(System.Type scriptType, string name);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Scene_CreateEntity(string name, ulong entityID);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Scene_DestroyEntity(ulong entityHandle);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Scene_CloneEntity(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Scene_GetActiveCameraID();

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Scene_GetIDFromTag(string tag);

		#endregion

		#region Entity

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern object Entity_GetInstance(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Entity_HasParent(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Entity Entity_GetParent(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Entity_SetParent(ulong entityID, ulong parentID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern Entity[] Entity_GetChildren(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Entity_HasComponent(ulong id, System.Type compType);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Entity_AddComponent(ulong id, System.Type compType);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Entity_RemoveComponent(ulong id, System.Type compType);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern object Entity_Instantiate(System.Type type, string name);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern object Entity_DestroyEntity(ulong entityID, bool destroyChildren);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Entity_CreateEntity(string name);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Entity_CloneEntity(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Entity_FindEntityByName(string name);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Entity_FindChildEntityByName(ulong entityID, string name, bool recusive);

		#endregion

		#region TagComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern string TagComponent_GetTag(ulong id);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TagComponent_SetTag(ulong id, string tag);

		#endregion

		#region TransformComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetTranslation(ulong id, out Vector3 translation);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetTranslation(ulong id, ref Vector3 translation);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetRotation(ulong id, out Vector3 rotation);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetRotation(ulong id, ref Vector3 rotation);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetScale(ulong id, out Vector3 scale);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetScale(ulong id, ref Vector3 scale);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetLocalTransform(ulong id, out Transform out_LocalTransform);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetLocalTransform(ulong id, ref Transform localTransform);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetWorldTransform(ulong id, out Transform out_WorldTransform);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetWorldTransform(ulong id, ref Transform worldTransform);

		#endregion

		#region SpriteRendererComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_GetColor(ulong id, out Color color);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_SetColor(ulong id, ref Color color);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern AssetHandle SpriteRendererComponent_GetTextureHandle(ulong id);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_SetTextureHandle(ulong id, AssetHandle textureHandle);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SpriteRendererComponent_GetTilingFactor(ulong id);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_SetTilingFactor(ulong id, float tilingFactor);

		#endregion

		#region CricleRendererComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleRendererComponent_GetColor(ulong id, out Color color);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleRendererComponent_SetColor(ulong id, ref Color color);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CircleRendererComponent_GetThickness(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleRendererComponent_SetThickness(ulong id, float thickness);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CircleRendererComponent_GetFade(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleRendererComponent_SetFade(ulong id, float fade);

		#endregion

		#region CameraComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_GetProjection(ulong id, out Matrix4 projection);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetProjection(ulong id, ref Matrix4 projection);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern CameraComponent.ProjectionType CameraComponent_GetProjectionType(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetProjectionType(ulong id, CameraComponent.ProjectionType projectionType);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspective(ulong id, float aspectratio, float fov, float clipnear, float clipfar);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographic(ulong id, float aspectratio, float zoom, float clipnear, float clipfar);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetAspectratio(ulong id);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetAspectratio(ulong id, float aspectratio);

		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetPerspectiveFOV(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspectiveFOV(ulong id, float fov);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetPerspectiveNear(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspectiveNear(ulong id, float near);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetPerspectiveFar(ulong id);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspectiveFar(ulong id, float far);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetOrthographicZoom(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographicZoom(ulong id, float zoom);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetOrthographicNear(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographicNear(ulong id, float near);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetOrthographicFar(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographicFar(ulong id, float far);

		#endregion

		#region Physics2D

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Physics2D_GetGravity(out Vector2 gravity);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Physics2D_SetGravity(ref Vector2 gravity);

		#endregion

		#region RigidBody2DComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern RigidBody2DType RigidBody2DComponent_GetBodyType(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetBodyType(ulong id, RigidBody2DType bodyType);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_GetTransform(ulong id, out RigidBody2DTransform transform);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetTransform(ulong id, ref RigidBody2DTransform transform);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetPosition(ulong id, ref Vector2 position);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetRotation(ulong id, float rotation);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_GetLocalCenter(ulong id, out Vector2 localCenter);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_GetWorldCenter(ulong id, out Vector2 worldCenter);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_GetLinearVelocity(ulong id, out Vector2 linearVelocity);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetLinearVelocity(ulong id, ref Vector2 linearVelocity);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBody2DComponent_GetAngularVelocity(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetAngularVelocity(ulong id, float angularVelocity);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBody2DComponent_GetGravityScale(ulong id);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetGravityScale(ulong id, float gravityScale);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBody2DComponent_GetLinearDamping(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetLinearDamping(ulong id, float linearDamping);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float RigidBody2DComponent_GetAngularDamping(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetAngularDamping(ulong id, float angularDamping);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsBullet(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetBullet(ulong id, bool bullet);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsSleepingAllowed(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetSleepingAllowed(ulong id, bool sleepingAllowed);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsAwake(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetAwake(ulong id, bool awake);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsEnabled(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetEnabled(ulong id, bool enabled);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool RigidBody2DComponent_IsFixedRotation(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_SetFixedRotation(ulong id, bool fixedRotation);



		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyForce(ulong id, ref Vector2 force, ref Vector2 point, PhysicsForce2DType forceType);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyForceToCenter(ulong id, ref Vector2 force, PhysicsForce2DType forceType);
		
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void RigidBody2DComponent_ApplyTorque(ulong id, float torque, PhysicsForce2DType forceType);

		#endregion

		#region BoxCollider2DComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxCollider2DComponent_SetSensor(ulong id, bool sensor);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool BoxCollider2DComponent_IsSensor(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxCollider2DComponent_SetDensity(ulong id, float density);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float BoxCollider2DComponent_GetDensity(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxCollider2DComponent_SetFriction(ulong id, float friction);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float BoxCollider2DComponent_GetFriction(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxCollider2DComponent_SetRestitution(ulong id, float restitution);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float BoxCollider2DComponent_GetRestitution(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxCollider2DComponent_SetRestitutionThreshold(ulong id, float restitutionThreshold);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float BoxCollider2DComponent_GetRestitutionThreshold(ulong id);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxCollider2DComponent_GetSize(ulong id, out Vector2 size);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxCollider2DComponent_SetSize(ulong id, ref Vector2 size);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxCollider2DComponent_GetOffset(ulong id, out Vector2 offset);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxCollider2DComponent_SetOffset(ulong id, ref Vector2 offset);
		

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float BoxCollider2DComponent_GetRotation(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void BoxCollider2DComponent_SetRotation(ulong id, float rotation);

		#endregion

		#region CircleCollider2DComponent

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleCollider2DComponent_SetSensor(ulong id, bool sensor);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CircleCollider2DComponent_IsSensor(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleCollider2DComponent_SetDensity(ulong id, float density);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CircleCollider2DComponent_GetDensity(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleCollider2DComponent_SetFriction(ulong id, float friction);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CircleCollider2DComponent_GetFriction(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleCollider2DComponent_SetRestitution(ulong id, float restitution);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CircleCollider2DComponent_GetRestitution(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleCollider2DComponent_SetRestitutionThreshold(ulong id, float restitutionThreshold);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CircleCollider2DComponent_GetRestitutionThreshold(ulong id);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CircleCollider2DComponent_GetRadius(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleCollider2DComponent_SetRadius(ulong id, float Radius);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleCollider2DComponent_GetOffset(ulong id, out Vector2 offset);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleCollider2DComponent_SetOffset(ulong id, Vector2 offset);


		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CircleCollider2DComponent_GetRotation(ulong id);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CircleCollider2DComponent_SetRotation(ulong id, float rotation);

		#endregion

		#region ResourceManager

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void ResourceManager_GetAssetHandleFromFilePath(string filePath, out AssetHandle assetHandle);

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
