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
		static void Glue();
		static void UnGlue();

		static void CallCollishionBegin(Entity entityA, Entity entityB);
		static void CallCollishionEnd(Entity entityA, Entity entityB);

		static void OnEvent(Event& event);

	private:
		static void RegisterComponents();
		static void RegsiterInternalCalls();

	};

	namespace InternalCalls {

		struct Vector2i
		{
			int x, y;
		};

		#pragma region Log

		enum class LogLevel : uint16_t
		{
			Trace = 0,
			Debug = 1,
			Info = 2,
			Warn = 3,
			Error = 4,
			Critical = 5
		};

		void Log_LogLevel(LogLevel level, MonoString* message);

		#pragma endregion

		#pragma region UUID

		UUID UUID_Generate();

		#pragma endregion

		#pragma region Input

		bool Input_KeyPressed(KeyCode key);
		bool Input_MouseButtonPressed(MouseButton::Type button);
		Vector2i Input_GetMousePos();
		Vector2i Input_GetMousePosGlobal();

		#pragma endregion

		#pragma region Matrix4

		void Matrix4_Inverse(glm::mat4* matrix, glm::mat4* out_Result);
		glm::mat4 Matrix4_Matrix4MulMatrix4(glm::mat4* lhs, glm::mat4* rhs);
		glm::vec4 Matrix4_Matrix4MulVector4(glm::mat4* lhs, glm::vec4* rhs);

		#pragma endregion

		#pragma region Scene

		MonoObject* Scene_InstantiateScript(MonoReflectionType* scriptType, MonoString* name);
		void Scene_CreateEntity(MonoString* name, UUID uuid, UUID* out_UUID);
		void Scene_DestroyEntity(UUID entityHandle);
		void Scene_CloneEntity(UUID entityHandle, UUID* out_UUID);
		MonoObject* Scene_GetScriptObject(UUID scriptEntityHandle);
		bool Scene_IsValidEntityHandle(UUID entityHandle);
		void Scene_GetActiveCameraUUID(UUID* out_UUID);
		void Scene_GetUUIDFromTag(MonoString* tag, UUID* out_UUID);
		Bounds2i Scene_GetViewportBounds();

		#pragma endregion

		#pragma region Entity

		enum class ComponentType : uint16_t
		{
			None = 0,
			ID = 1,
			Tag = 2,
			Transform = 3,
			SpriteRenderer = 4,
			Camera = 5,
			RigidBody2D = 6,
			BoxCollider2D = 7,
			CircleCollider2D = 8,
		};

		MonoString* Entity_GetName(UUID entityHandle);
		void Entity_SetName(UUID entityHandle, MonoString* name);
		bool Entity_HasComponent(UUID entityHandle, MonoReflectionType* type);
		void Entity_AddComponent(UUID entityHandle, MonoReflectionType* type);

		#pragma endregion

		#pragma region TransformComponent

		glm::vec3 TransformComponent_GetTranslation(UUID uuid);
		void TransformComponent_SetTranslation(UUID uuid, glm::vec3 translation);
		
		glm::vec3 TransformComponent_GetRotation(UUID uuid);
		void TransformComponent_SetRotation(UUID uuid, glm::vec3 rotation);
		
		glm::vec3 TransformComponent_GetScaling(UUID uuid);
		void TransformComponent_SetScaling(UUID uuid, glm::vec3 scaling);

		#pragma endregion

		#pragma region SpriteRendererComponent

		glm::vec4 SpriteRendererComponent_GetColor(UUID entityHandle);
		void SpriteRendererComponent_SetColor(UUID entityHandle, glm::vec4 color);
		AssetHandle SpriteRendererComponent_GetTextureHandle(UUID entityHandle);
		void SpriteRendererComponent_SetTextureHandle(UUID entityHandle, AssetHandle textureHandle);
		float SpriteRendererComponent_GetTilingFactor(UUID entityHandle);
		void SpriteRendererComponent_SetTilingFactor(UUID entityHandle, float tilingFactor);

		#pragma endregion

		#pragma region CricleRendererComponent

		glm::vec4 CircleRendererComponent_GetColor(UUID entityHandle);
		void CircleRendererComponent_SetColor(UUID entityHandle, glm::vec4 color);
		float CircleRendererComponent_GetThickness(UUID entityHandle);
		void CircleRendererComponent_SetThickness(UUID entityHandle, float thickness);
		float CircleRendererComponent_GetFade(UUID entityHandle);
		void CircleRendererComponent_SetFade(UUID entityHandle, float fade);

		#pragma endregion

		#pragma region CameraComponent

		glm::mat4 CameraComponent_GetProjection(UUID entityHandle);
		void CameraComponent_SetProjection(UUID entityHandle, glm::mat4 projection);

		SceneCamera::Projection CameraComponent_GetProjectionType(UUID entityHandle);
		void CameraComponent_SetProjectionType(UUID entityHandle, SceneCamera::Projection projectionType);

		void CameraComponent_SetPerspective(UUID entityHandle, float aspectratio, float fov, float clipnear, float clipfar);
		void CameraComponent_SetOrthographic(UUID entityHandle, float aspectratio, float zoom, float clipnear, float clipfar);

		float CameraComponent_GetAspectratio(UUID entityHandle);
		void CameraComponent_SetAspectratio(UUID entityHandle, float aspectratio);

		float CameraComponent_GetPerspectiveFOV(UUID entityHandle);
		void CameraComponent_SetPerspectiveFOV(UUID entityHandle, float fov);

		float CameraComponent_GetPerspectiveNear(UUID entityHandle);
		void CameraComponent_SetPerspectiveNear(UUID entityHandle, float clipnear);

		float CameraComponent_GetPerspectiveFar(UUID entityHandle);
		void CameraComponent_SetPerspectiveFar(UUID entityHandle, float clipfar);

		float CameraComponent_GetOrthographicZoom(UUID entityHandle);
		void CameraComponent_SetOrthographicZoom(UUID entityHandle, float zoom);

		float CameraComponent_GetOrthographicNear(UUID entityHandle);
		void CameraComponent_SetOrthographicNear(UUID entityHandle, float clipnear);

		float CameraComponent_GetOrthographicFar(UUID entityHandle);
		void CameraComponent_SetOrthographicFar(UUID entityHandle, float clipfar);

		#pragma endregion

		#pragma region RigidBody2DComponent

		struct RigidBody2DTransform
		{
			glm::vec2 Position;
			float Angle;
		};

		struct Vector2
		{
			float x, y;
		};

		enum class PhysicsForce2DType
		{
			Force = 0,
			Impulse = 1
		};

		void* RigidBody2DComponent_GetNativeHandle(UUID owner);
		RigidBody2DComponent::BodyType RigidBody2DComponent_GetBodyType(void* nativeHandle);
		void RigidBody2DComponent_SetBodyType(void* nativeHandle, RigidBody2DComponent::BodyType bodyType);
		RigidBody2DTransform RigidBody2DComponent_GetTransform(void* nativeHandle);
		void RigidBody2DComponent_SetTransform(void* nativeHandle, RigidBody2DTransform* transform);
		void RigidBody2DComponent_SetPosition(void* nativeHandle, glm::vec2 position);
		void RigidBody2DComponent_SetRotation(void* nativeHandle, float rotation);
		Vector2 RigidBody2DComponent_GetLocalCenter(void* nativeHandle);
		Vector2 RigidBody2DComponent_GetWorldCenter(void* nativeHandle);
		Vector2 RigidBody2DComponent_GetLinearVelocity(void* nativeHandle);
		void RigidBody2DComponent_SetLinearVelocity(void* nativeHandle, glm::vec2* linearVelocity);
		float RigidBody2DComponent_GetAngularVelocity(void* nativeHandle);
		void RigidBody2DComponent_SetAngularVelocity(void* nativeHandle, float angularVelocity);
		void RigidBody2DComponent_ApplyForce(void* nativeHandle, glm::vec2* force, glm::vec2* point, PhysicsForce2DType forceType);
		void RigidBody2DComponent_ApplyForceToCenter(void* nativeHandle, glm::vec2* force, PhysicsForce2DType forceType);
		void RigidBody2DComponent_ApplyTorque(void* nativeHandle, float torque, PhysicsForce2DType forceType);
		float RigidBody2DComponent_GetGravityScale(void* nativeHandle);
		void RigidBody2DComponent_SetGravityScale(void* nativeHandle, float gravityScale);
		float RigidBody2DComponent_GetLinearDamping(void* nativeHandle);
		void RigidBody2DComponent_SetLinearDamping(void* nativeHandle, float linearDamping);
		float RigidBody2DComponent_GetAngularDamping(void* nativeHandle);
		void RigidBody2DComponent_SetAngularDamping(void* nativeHandle, float angularDamping);
		bool RigidBody2DComponent_IsBullet(void* nativeHandle);
		void RigidBody2DComponent_SetBullet(void* nativeHandle, bool bullet);
		bool RigidBody2DComponent_IsSleepingAllowed(void* nativeHandle);
		void RigidBody2DComponent_SetSleepingAllowed(void* nativeHandle, bool sleepingAllowed);
		bool RigidBody2DComponent_IsAwake(void* nativeHandle);
		void RigidBody2DComponent_SetAwake(void* nativeHandle, bool awake);
		bool RigidBody2DComponent_IsEnabled(void* nativeHandle);
		void RigidBody2DComponent_SetEnabled(void* nativeHandle, bool enabled);
		bool RigidBody2DComponent_IsFixedRotation(void* nativeHandle);
		void RigidBody2DComponent_SetFixedRotation(void* nativeHandle, bool fixedRotation);

		#pragma endregion

		#pragma region PhysicsCollider2D

		void PhysicsCollider2D_SetSensor(void* nativeHandle, bool sensor);
		bool PhysicsCollider2D_IsSensor(void* nativeHandle);
		void PhysicsCollider2D_SetDensity(void* nativeHandle, float density);
		float PhysicsCollider2D_GetDensity(void* nativeHandle);
		void PhysicsCollider2D_SetFriction(void* nativeHandle, float friction);
		float PhysicsCollider2D_GetFriction(void* nativeHandle);
		void PhysicsCollider2D_SetRestitution(void* nativeHandle, float restitution);
		float PhysicsCollider2D_GetRestitution(void* nativeHandle);
		void PhysicsCollider2D_SetRestitutionThreshold(void* nativeHandle, float restitutionThreshold);
		float PhysicsCollider2D_GetRestitutionThreshold(void* nativeHandle);

		#pragma endregion

		#pragma region BoxCollider2DComponent

		void* BoxCollider2DComponent_GetNativeHandle(UUID owner);

		glm::vec2 BoxCollider2DComponent_GetSize(UUID owner);
		void BoxCollider2DComponent_SetSize(UUID owner, glm::vec2 size);
		glm::vec2 BoxCollider2DComponent_GetOffset(UUID owner);
		void BoxCollider2DComponent_SetOffset(UUID owner, glm::vec2 offset);
		float BoxCollider2DComponent_GetRotation(UUID owner);
		void BoxCollider2DComponent_SetRotation(UUID owner, float rotation);

		#pragma endregion

		#pragma region CircleCollider2DComponent

		void* CircleCollider2DComponent_GetNativeHandle(UUID owner);

		float CircleCollider2DComponent_GetRadius(UUID owner);
		void CircleCollider2DComponent_SetRadius(UUID owner, float Radius);
		glm::vec2 CircleCollider2DComponent_GetOffset(UUID owner);
		void CircleCollider2DComponent_SetOffset(UUID owner, glm::vec2 offset);
		float CircleCollider2DComponent_GetRotation(UUID owner);
		void CircleCollider2DComponent_SetRotation(UUID owner, float rotation);

		#pragma endregion

		#pragma region ResoureManager

		void ResourceManager_GetAssetHandleFromFilePath(MonoString* filePath, AssetHandle* out_AssetHandle);

		#pragma endregion

	}

}
