#pragma once

#include "Shark/Core/UUID.h"

#include "Shark/Asset/Asset.h"

#include "Shark/Scene/Components/TransformComponent.h"
#include "Shark/Scene/Components/RigidBody2DComponent.h"
#include "Shark/Scene/SceneCamera.h"
#include "Shark/Input/Input.h"

#include "Shark/Event/Event.h"
#include "Shark/Math/Bounds2.h"

#include <mono/metadata/object.h>

namespace Shark {

	class Entity;

	class MonoGlue
	{
	public:
		static void Init();
		static void Shutdown();

		static void CallCollishionBegin(Entity entityA, Entity entityB);
		static void CallCollishionEnd(Entity entityA, Entity entityB);

		static void OnEvent(Event& event);

	private:
		static void RegisterComponents();
		static void RegsiterInternalCalls();

	};

	namespace InternalCalls {

		#pragma region Application

		uint32_t Application_GetWidth();
		uint32_t Application_GetHeight();

		#pragma endregion

		#pragma region Log

		void Log_LogLevel(Log::Level level, MonoString* message);

		#pragma endregion

		#pragma region Input

		bool Input_KeyPressed(KeyCode key);
		bool Input_MouseButtonPressed(MouseButton::Type button);
		bool Input_GetMousePos(glm::ivec2* out_MousePos);

		#pragma endregion

		#pragma region Matrix4

		void Matrix4_Inverse(glm::mat4* matrix, glm::mat4* out_Result);
		glm::mat4 Matrix4_Matrix4MulMatrix4(glm::mat4* lhs, glm::mat4* rhs);
		glm::vec4 Matrix4_Matrix4MulVector4(glm::mat4* lhs, glm::vec4* rhs);

		#pragma endregion

		#pragma region Scene

		MonoObject* Scene_InstantiateScript(MonoReflectionType* scriptType, MonoString* name);
		void Scene_CreateEntity(MonoString* name, uint64_t entityID, uint64_t* out_EntityID);
		void Scene_DestroyEntity(uint64_t entityID);
		void Scene_CloneEntity(uint64_t entityID, UUID* out_UUID);
		MonoObject* Scene_GetScriptObject(uint64_t scriptEntityID);
		bool Scene_IsValidEntityHandle(uint64_t entityID);
		bool Scene_GetActiveCameraUUID(uint64_t* out_CameraID);
		bool Scene_GetUUIDFromTag(MonoString* tag, uint64_t* out_EntityID);

		#pragma endregion

		#pragma region Entity

		bool Entity_HasComponent(uint64_t id, MonoReflectionType* type);
		void Entity_AddComponent(uint64_t id, MonoReflectionType* type);
		void Entity_RemoveComponent(uint64_t id, MonoReflectionType* type);

		#pragma endregion

		#pragma region TagComponent

		bool TagComponent_GetTag(uint64_t id, MonoString** out_Tag);
		bool TagComponent_SetTag(uint64_t id, MonoString* tag);

		#pragma endregion

		#pragma region TransformComponent

		bool TransformComponent_GetTranslation(uint64_t id, glm::vec3* out_Translation);
		bool TransformComponent_SetTranslation(uint64_t id, glm::vec3* translation);
		
		bool TransformComponent_GetRotation(uint64_t id, glm::vec3* out_Rotation);
		bool TransformComponent_SetRotation(uint64_t id, glm::vec3* rotation);
		
		bool TransformComponent_GetScale(uint64_t id, glm::vec3* out_Scaling);
		bool TransformComponent_SetScale(uint64_t id, glm::vec3* scaling);

		bool TransformComponent_GetLocalTransform(uint64_t id, TransformComponent* out_LocalTransform);
		bool TransformComponent_SetLocalTransform(uint64_t id, TransformComponent* localTransform);
		
		bool TransformComponent_GetWorldTransform(uint64_t id, TransformComponent* out_WorldTransform);
		bool TransformComponent_SetWorldTransform(uint64_t id, TransformComponent* worldTransform);

		#pragma endregion

		#pragma region SpriteRendererComponent

		bool SpriteRendererComponent_GetColor(uint64_t id, glm::vec4* out_Color);
		bool SpriteRendererComponent_SetColor(uint64_t id, glm::vec4* color);

		bool SpriteRendererComponent_GetTextureHandle(uint64_t id, AssetHandle* out_TextureHandle);
		bool SpriteRendererComponent_SetTextureHandle(uint64_t id, AssetHandle* textureHandle);

		bool SpriteRendererComponent_GetTilingFactor(uint64_t id, float* out_TilingFactor);
		bool SpriteRendererComponent_SetTilingFactor(uint64_t id, float tilingFactor);

		#pragma endregion

		#pragma region CricleRendererComponent

		bool CircleRendererComponent_GetColor(uint64_t id, glm::vec4* out_Color);
		bool CircleRendererComponent_SetColor(uint64_t id, glm::vec4* color);

		bool CircleRendererComponent_GetThickness(uint64_t id, float* out_Thickness);
		bool CircleRendererComponent_SetThickness(uint64_t id, float thickness);

		bool CircleRendererComponent_GetFade(uint64_t id, float* out_Fade);
		bool CircleRendererComponent_SetFade(uint64_t id, float fade);

		#pragma endregion

		#pragma region CameraComponent

		bool CameraComponent_GetProjection(uint64_t id, glm::mat4* out_Projection);
		bool CameraComponent_SetProjection(uint64_t id, glm::mat4* projection);

		bool CameraComponent_GetProjectionType(uint64_t id, SceneCamera::Projection* out_ProjectionType);
		bool CameraComponent_SetProjectionType(uint64_t id, SceneCamera::Projection projectionType);

		bool CameraComponent_SetPerspective(uint64_t id, float aspectratio, float fov, float clipnear, float clipfar);
		bool CameraComponent_SetOrthographic(uint64_t id, float aspectratio, float zoom, float clipnear, float clipfar);

		bool CameraComponent_GetAspectratio(uint64_t id, float* out_Aspectratio);
		bool CameraComponent_SetAspectratio(uint64_t id, float aspectratio);

		bool CameraComponent_GetPerspectiveFOV(uint64_t id, float* out_FOV);
		bool CameraComponent_SetPerspectiveFOV(uint64_t id, float fov);

		bool CameraComponent_GetPerspectiveNear(uint64_t id, float* out_Near);
		bool CameraComponent_SetPerspectiveNear(uint64_t id, float clipnear);

		bool CameraComponent_GetPerspectiveFar(uint64_t id, float* out_Far);
		bool CameraComponent_SetPerspectiveFar(uint64_t id, float clipfar);

		bool CameraComponent_GetOrthographicZoom(uint64_t id, float* out_Zoom);
		bool CameraComponent_SetOrthographicZoom(uint64_t id, float zoom);

		bool CameraComponent_GetOrthographicNear(uint64_t id, float* out_Near);
		bool CameraComponent_SetOrthographicNear(uint64_t id, float clipnear);

		bool CameraComponent_GetOrthographicFar(uint64_t id, float* out_Far);
		bool CameraComponent_SetOrthographicFar(uint64_t id, float clipfar);

		#pragma endregion

		#pragma region RigidBody2DComponent

		struct RigidBody2DTransform
		{
			glm::vec2 Position;
			float Angle;
		};

		enum class PhysicsForce2DType
		{
			Force = 0,
			Impulse = 1
		};

		bool RigidBody2DComponent_GetBodyType(uint64_t id, RigidBody2DComponent::BodyType* out_BodyType);
		bool RigidBody2DComponent_SetBodyType(uint64_t id, RigidBody2DComponent::BodyType bodyType);
		bool RigidBody2DComponent_GetTransform(uint64_t id, RigidBody2DTransform* out_Transform);
		bool RigidBody2DComponent_SetTransform(uint64_t id, RigidBody2DTransform* transform);
		bool RigidBody2DComponent_SetPosition(uint64_t id, glm::vec2* position);
		bool RigidBody2DComponent_SetRotation(uint64_t id, float rotation);
		bool RigidBody2DComponent_GetLocalCenter(uint64_t id, glm::vec2* out_LocalCenter);
		bool RigidBody2DComponent_GetWorldCenter(uint64_t id, glm::vec2* out_WorldCenter);
		bool RigidBody2DComponent_GetLinearVelocity(uint64_t id, glm::vec2* out_LinearVelocity);
		bool RigidBody2DComponent_SetLinearVelocity(uint64_t id, glm::vec2* linearVelocity);
		bool RigidBody2DComponent_GetAngularVelocity(uint64_t id, float* out_AngularVelocity);
		bool RigidBody2DComponent_SetAngularVelocity(uint64_t id, float angularVelocity);
		bool RigidBody2DComponent_ApplyForce(uint64_t id, glm::vec2* force, glm::vec2* point, PhysicsForce2DType forceType);
		bool RigidBody2DComponent_ApplyForceToCenter(uint64_t id, glm::vec2* force, PhysicsForce2DType forceType);
		bool RigidBody2DComponent_ApplyTorque(uint64_t id, float torque, PhysicsForce2DType forceType);
		bool RigidBody2DComponent_GetGravityScale(uint64_t id, float* out_GravityScale);
		bool RigidBody2DComponent_SetGravityScale(uint64_t id, float gravityScale);
		bool RigidBody2DComponent_GetLinearDamping(uint64_t id, float* out_LinearDamping);
		bool RigidBody2DComponent_SetLinearDamping(uint64_t id, float linearDamping);
		bool RigidBody2DComponent_GetAngularDamping(uint64_t id, float* out_AngularDamping);
		bool RigidBody2DComponent_SetAngularDamping(uint64_t id, float angularDamping);
		bool RigidBody2DComponent_IsBullet(uint64_t id, bool* out_IsBullet);
		bool RigidBody2DComponent_SetBullet(uint64_t id, bool bullet);
		bool RigidBody2DComponent_IsSleepingAllowed(uint64_t id, bool* out_SleepingAllowed);
		bool RigidBody2DComponent_SetSleepingAllowed(uint64_t id, bool sleepingAllowed);
		bool RigidBody2DComponent_IsAwake(uint64_t id, bool* out_IsAwake);
		bool RigidBody2DComponent_SetAwake(uint64_t id, bool awake);
		bool RigidBody2DComponent_IsEnabled(uint64_t id, bool* out_IsEnabled);
		bool RigidBody2DComponent_SetEnabled(uint64_t id, bool enabled);
		bool RigidBody2DComponent_IsFixedRotation(uint64_t id, bool* out_FixedRotation);
		bool RigidBody2DComponent_SetFixedRotation(uint64_t id, bool fixedRotation);

		#pragma endregion

		#pragma region BoxCollider2DComponent

		bool BoxCollider2DComponent_SetSensor(uint64_t id, bool sensor);
		bool BoxCollider2DComponent_IsSensor(uint64_t id, bool* out_IsSensor);
		bool BoxCollider2DComponent_SetDensity(uint64_t id, float density);
		bool BoxCollider2DComponent_GetDensity(uint64_t id, bool* out_Density);
		bool BoxCollider2DComponent_SetFriction(uint64_t id, float friction);
		bool BoxCollider2DComponent_GetFriction(uint64_t id, float* out_Friction);
		bool BoxCollider2DComponent_SetRestitution(uint64_t id, float restitution);
		bool BoxCollider2DComponent_GetRestitution(uint64_t id, float* out_Restitution);
		bool BoxCollider2DComponent_SetRestitutionThreshold(uint64_t id, float restitutionThreshold);
		bool BoxCollider2DComponent_GetRestitutionThreshold(uint64_t id, float* out_RestitutionThreshold);

		bool BoxCollider2DComponent_GetSize(uint64_t id, glm::vec2* out_Size);
		bool BoxCollider2DComponent_SetSize(uint64_t id, glm::vec2* size);
		bool BoxCollider2DComponent_GetOffset(uint64_t id, glm::vec2* out_Offset);
		bool BoxCollider2DComponent_SetOffset(uint64_t id, glm::vec2* offset);
		bool BoxCollider2DComponent_GetRotation(uint64_t id, float* out_Rotation);
		bool BoxCollider2DComponent_SetRotation(uint64_t id, float rotation);

		#pragma endregion

		#pragma region CircleCollider2DComponent

		bool CircleCollider2DComponent_SetSensor(uint64_t id, bool sensor);
		bool CircleCollider2DComponent_IsSensor(uint64_t id, bool* out_IsSensor);
		bool CircleCollider2DComponent_SetDensity(uint64_t id, float density);
		bool CircleCollider2DComponent_GetDensity(uint64_t id, bool* out_Density);
		bool CircleCollider2DComponent_SetFriction(uint64_t id, float friction);
		bool CircleCollider2DComponent_GetFriction(uint64_t id, float* out_Friction);
		bool CircleCollider2DComponent_SetRestitution(uint64_t id, float restitution);
		bool CircleCollider2DComponent_GetRestitution(uint64_t id, float* out_Restitution);
		bool CircleCollider2DComponent_SetRestitutionThreshold(uint64_t id, float restitutionThreshold);
		bool CircleCollider2DComponent_GetRestitutionThreshold(uint64_t id, float* out_RestitutionThreshold);

		bool CircleCollider2DComponent_GetRadius(uint64_t id, float* out_Radius);
		bool CircleCollider2DComponent_SetRadius(uint64_t id, float Radius);
		bool CircleCollider2DComponent_GetOffset(uint64_t id, glm::vec2* out_Offsets);
		bool CircleCollider2DComponent_SetOffset(uint64_t id, glm::vec2* offset);
		bool CircleCollider2DComponent_GetRotation(uint64_t id, float* out_Rotation);
		bool CircleCollider2DComponent_SetRotation(uint64_t id, float rotation);

		#pragma endregion

		#pragma region ResoureManager

		bool ResourceManager_GetAssetHandleFromFilePath(MonoString* filePath, AssetHandle* out_AssetHandle);

		#pragma endregion

		#pragma region EditorUI

		bool EditorUI_BeginWindow(MonoString* windowTitle);
		void EditorUI_EndWindow();
		void EditorUI_Text(MonoString* text);
		void EditorUI_NewLine();
		void EditorUI_Separator();

		#pragma endregion


	}

}
