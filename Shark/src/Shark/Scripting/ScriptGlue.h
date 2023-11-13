#pragma once

#include "Shark/Core/UUID.h"

#include "Shark/Asset/Asset.h"

#include "Shark/Scene/Components.h"
#include "Shark/Scene/Physics2DScene.h"
#include "Shark/Scene/SceneCamera.h"
#include "Shark/Input/Input.h"

#include "Shark/Event/Event.h"

extern "C" {
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoArray MonoArray;
	typedef struct _MonoString MonoString;
	typedef struct _MonoReflectionType MonoReflectionType;
}

namespace Shark {

	class Entity;

	class ScriptGlue
	{
	public:
		static void Init();
		static void Shutdown();

		// Calls the corresponding function on entityA (not on entityB!) 
		static void CallCollishionBegin(Entity entityA, Entity entityB, Collider2DType colliderType, bool isSensor);
		static void CallCollishionEnd(Entity entityA, Entity entityB, Collider2DType colliderType, bool isSensor);

	private:
		static void RegisterComponents();
		static void RegisterInternalCalls();

	};

	namespace InternalCalls {

		#pragma region Application

		uint32_t Application_GetWidth();
		uint32_t Application_GetHeight();

		#pragma endregion

		#pragma region Log

		void Log_LogMessage(Log::Level level, MonoString* message);

		#pragma endregion

		#pragma region Input

		bool Input_IsKeyStateSet(KeyCode key, KeyState keyState);
		bool Input_IsMouseStateSet(MouseButton button, MouseState mouseState);
		float Input_GetMouseScroll();
		void Input_GetMousePos(glm::ivec2* out_MousePos);

		#pragma endregion

		#pragma region Matrix4

		void Matrix4_Inverse(glm::mat4* matrix, glm::mat4* out_Result);
		void Matrix4_Matrix4MulMatrix4(glm::mat4* lhs, glm::mat4* rhs, glm::mat4* out_Result);
		void Matrix4_Matrix4MulVector4(glm::mat4* lhs, glm::vec4* rhs, glm::vec4* out_Result);

		#pragma endregion

		#pragma region Entity

		MonoObject* Entity_GetInstance(uint64_t entityID);
		bool Entity_HasParent(uint64_t entityID);
		MonoObject* Entity_GetParent(uint64_t entityID);
		void Entity_SetParent(uint64_t entityID, uint64_t parentID);
		MonoArray* Entity_GetChildren(uint64_t entityID);
		bool Entity_HasComponent(uint64_t id, MonoReflectionType* type);
		void Entity_AddComponent(uint64_t id, MonoReflectionType* type);
		void Entity_RemoveComponent(uint64_t id, MonoReflectionType* type);
		MonoObject* Entity_Instantiate(MonoReflectionType* type, MonoString* name);
		void Entity_DestroyEntity(uint64_t entityID, bool destroyChildren);
		uint64_t Entity_CreateEntity(MonoString* name);
		uint64_t Entity_CloneEntity(uint64_t entityID);
		uint64_t Entity_FindEntityByName(MonoString* name);
		uint64_t Entity_FindChildEntityByName(uint64_t, MonoString* name, bool recusive);

		#pragma endregion

		#pragma region TagComponent

		MonoString* TagComponent_GetTag(uint64_t id);
		void TagComponent_SetTag(uint64_t id, MonoString* tag);

		#pragma endregion

		#pragma region TransformComponent

		void TransformComponent_GetTranslation(uint64_t id, glm::vec3* out_Translation);
		void TransformComponent_SetTranslation(uint64_t id, glm::vec3* translation);
		
		void TransformComponent_GetRotation(uint64_t id, glm::vec3* out_Rotation);
		void TransformComponent_SetRotation(uint64_t id, glm::vec3* rotation);
		
		void TransformComponent_GetScale(uint64_t id, glm::vec3* out_Scaling);
		void TransformComponent_SetScale(uint64_t id, glm::vec3* scaling);

		void TransformComponent_GetLocalTransform(uint64_t id, TransformComponent* out_LocalTransform);
		void TransformComponent_SetLocalTransform(uint64_t id, TransformComponent* localTransform);
		
		void TransformComponent_GetWorldTransform(uint64_t id, TransformComponent* out_WorldTransform);
		void TransformComponent_SetWorldTransform(uint64_t id, TransformComponent* worldTransform);

		#pragma endregion

		#pragma region SpriteRendererComponent

		void SpriteRendererComponent_GetColor(uint64_t id, glm::vec4* out_Color);
		void SpriteRendererComponent_SetColor(uint64_t id, glm::vec4* color);

		AssetHandle SpriteRendererComponent_GetTextureHandle(uint64_t id);
		void SpriteRendererComponent_SetTextureHandle(uint64_t id, AssetHandle textureHandle);

		float SpriteRendererComponent_GetTilingFactor(uint64_t id);
		void SpriteRendererComponent_SetTilingFactor(uint64_t id, float tilingFactor);

		#pragma endregion

		#pragma region CricleRendererComponent

		void CircleRendererComponent_GetColor(uint64_t id, glm::vec4* out_Color);
		void CircleRendererComponent_SetColor(uint64_t id, glm::vec4* color);

		float CircleRendererComponent_GetThickness(uint64_t id);
		void CircleRendererComponent_SetThickness(uint64_t id, float thickness);

		float CircleRendererComponent_GetFade(uint64_t id);
		void CircleRendererComponent_SetFade(uint64_t id, float fade);

		#pragma endregion

		#pragma region CameraComponent

		void CameraComponent_GetProjection(uint64_t id, glm::mat4* out_Projection);
		void CameraComponent_SetProjection(uint64_t id, glm::mat4* projection);

		SceneCamera::Projection CameraComponent_GetProjectionType(uint64_t id);
		void CameraComponent_SetProjectionType(uint64_t id, SceneCamera::Projection projectionType);

		void CameraComponent_SetPerspective(uint64_t id, float aspectratio, float fov, float clipnear, float clipfar);
		void CameraComponent_SetOrthographic(uint64_t id, float aspectratio, float zoom, float clipnear, float clipfar);

		float CameraComponent_GetAspectratio(uint64_t id);
		void CameraComponent_SetAspectratio(uint64_t id, float aspectratio);

		float CameraComponent_GetPerspectiveFOV(uint64_t id);
		void CameraComponent_SetPerspectiveFOV(uint64_t id, float fov);

		float CameraComponent_GetPerspectiveNear(uint64_t id);
		void CameraComponent_SetPerspectiveNear(uint64_t id, float clipnear);

		float CameraComponent_GetPerspectiveFar(uint64_t id);
		void CameraComponent_SetPerspectiveFar(uint64_t id, float clipfar);

		float CameraComponent_GetOrthographicZoom(uint64_t id);
		void CameraComponent_SetOrthographicZoom(uint64_t id, float zoom);

		float CameraComponent_GetOrthographicNear(uint64_t id);
		void CameraComponent_SetOrthographicNear(uint64_t id, float clipnear);

		float CameraComponent_GetOrthographicFar(uint64_t id);
		void CameraComponent_SetOrthographicFar(uint64_t id, float clipfar);

		#pragma endregion

		#pragma region Physics2D

		void Physics2D_GetGravity(glm::vec2* out_Gravity);
		void Physics2D_SetGravity(glm::vec2* gravity);

		bool Physics2D_GetAllowSleep();
		void Physics2D_SetAllowSleep(bool allowSleep);

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

		RigidBody2DComponent::BodyType RigidBody2DComponent_GetBodyType(uint64_t id);
		void RigidBody2DComponent_SetBodyType(uint64_t id, RigidBody2DComponent::BodyType bodyType);
		void RigidBody2DComponent_GetTransform(uint64_t id, RigidBody2DTransform* out_Transform);
		void RigidBody2DComponent_SetTransform(uint64_t id, RigidBody2DTransform* transform);
		void RigidBody2DComponent_SetPosition(uint64_t id, glm::vec2* position);
		void RigidBody2DComponent_SetRotation(uint64_t id, float rotation);
		void RigidBody2DComponent_GetLocalCenter(uint64_t id, glm::vec2* out_LocalCenter);
		void RigidBody2DComponent_GetWorldCenter(uint64_t id, glm::vec2* out_WorldCenter);
		void RigidBody2DComponent_GetLinearVelocity(uint64_t id, glm::vec2* out_LinearVelocity);
		void RigidBody2DComponent_SetLinearVelocity(uint64_t id, glm::vec2* linearVelocity);
		float RigidBody2DComponent_GetAngularVelocity(uint64_t id);
		void RigidBody2DComponent_SetAngularVelocity(uint64_t id, float angularVelocity);
		void RigidBody2DComponent_ApplyForce(uint64_t id, glm::vec2* force, glm::vec2* point, PhysicsForce2DType forceType);
		void RigidBody2DComponent_ApplyForceToCenter(uint64_t id, glm::vec2* force, PhysicsForce2DType forceType);
		void RigidBody2DComponent_ApplyTorque(uint64_t id, float torque, PhysicsForce2DType forceType);
		float RigidBody2DComponent_GetGravityScale(uint64_t id);
		void RigidBody2DComponent_SetGravityScale(uint64_t id, float gravityScale);
		float RigidBody2DComponent_GetLinearDamping(uint64_t id);
		void RigidBody2DComponent_SetLinearDamping(uint64_t id, float linearDamping);
		float RigidBody2DComponent_GetAngularDamping(uint64_t id);
		void RigidBody2DComponent_SetAngularDamping(uint64_t id, float angularDamping);
		bool RigidBody2DComponent_IsBullet(uint64_t id);
		void RigidBody2DComponent_SetBullet(uint64_t id, bool bullet);
		bool RigidBody2DComponent_IsSleepingAllowed(uint64_t id);
		void RigidBody2DComponent_SetSleepingAllowed(uint64_t id, bool sleepingAllowed);
		bool RigidBody2DComponent_IsAwake(uint64_t id);
		void RigidBody2DComponent_SetAwake(uint64_t id, bool awake);
		bool RigidBody2DComponent_IsEnabled(uint64_t id);
		void RigidBody2DComponent_SetEnabled(uint64_t id, bool enabled);
		bool RigidBody2DComponent_IsFixedRotation(uint64_t id);
		void RigidBody2DComponent_SetFixedRotation(uint64_t id, bool fixedRotation);

		#pragma endregion

		#pragma region BoxCollider2DComponent

		void BoxCollider2DComponent_SetSensor(uint64_t id, bool sensor);
		bool BoxCollider2DComponent_IsSensor(uint64_t id);
		void BoxCollider2DComponent_SetDensity(uint64_t id, float density);
		float BoxCollider2DComponent_GetDensity(uint64_t id);
		void BoxCollider2DComponent_SetFriction(uint64_t id, float friction);
		float BoxCollider2DComponent_GetFriction(uint64_t id);
		void BoxCollider2DComponent_SetRestitution(uint64_t id, float restitution);
		float BoxCollider2DComponent_GetRestitution(uint64_t id);
		void BoxCollider2DComponent_SetRestitutionThreshold(uint64_t id, float restitutionThreshold);
		float BoxCollider2DComponent_GetRestitutionThreshold(uint64_t id);

		void BoxCollider2DComponent_GetSize(uint64_t id, glm::vec2* out_Size);
		void BoxCollider2DComponent_SetSize(uint64_t id, glm::vec2* size);
		void BoxCollider2DComponent_GetOffset(uint64_t id, glm::vec2* out_Offset);
		void BoxCollider2DComponent_SetOffset(uint64_t id, glm::vec2* offset);
		float BoxCollider2DComponent_GetRotation(uint64_t id);
		void BoxCollider2DComponent_SetRotation(uint64_t id, float rotation);

		#pragma endregion

		#pragma region CircleCollider2DComponent

		void CircleCollider2DComponent_SetSensor(uint64_t id, bool sensor);
		bool CircleCollider2DComponent_IsSensor(uint64_t id);
		void CircleCollider2DComponent_SetDensity(uint64_t id, float density);
		float CircleCollider2DComponent_GetDensity(uint64_t id);
		void CircleCollider2DComponent_SetFriction(uint64_t id, float friction);
		float CircleCollider2DComponent_GetFriction(uint64_t id);
		void CircleCollider2DComponent_SetRestitution(uint64_t id, float restitution);
		float CircleCollider2DComponent_GetRestitution(uint64_t id);
		void CircleCollider2DComponent_SetRestitutionThreshold(uint64_t id, float restitutionThreshold);
		float CircleCollider2DComponent_GetRestitutionThreshold(uint64_t id);

		float CircleCollider2DComponent_GetRadius(uint64_t id);
		void CircleCollider2DComponent_SetRadius(uint64_t id, float Radius);
		void CircleCollider2DComponent_GetOffset(uint64_t id, glm::vec2* out_Offsets);
		void CircleCollider2DComponent_SetOffset(uint64_t id, glm::vec2* offset);
		float CircleCollider2DComponent_GetRotation(uint64_t id);
		void CircleCollider2DComponent_SetRotation(uint64_t id, float rotation);

		#pragma endregion

		#pragma region ResoureManager

		void ResourceManager_GetAssetHandleFromFilePath(MonoString* filePath, AssetHandle* out_AssetHandle);

		#pragma endregion

		// TODO(moro): Remove EditorUI
		#pragma region EditorUI

		bool EditorUI_BeginWindow(MonoString* windowTitle);
		void EditorUI_EndWindow();
		void EditorUI_Text(MonoString* text);
		void EditorUI_NewLine();
		void EditorUI_Separator();

		#pragma endregion


	}

}
