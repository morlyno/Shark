#pragma once

#include "Shark/Core/UUID.h"
#include "Shark/Scene/Components/TransformComponent.h"
#include "Shark/Scene/SceneCamera.h"
#include "Shark/Core/Input.h"

#include <mono/metadata/object.h>

namespace Shark {

	class MonoGlue
	{
	public:
		static void Glue();

	public:
		static void RegisterComponents();
		static void RegsiterInternalCalls();

	};

	namespace InternalCalls {

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
		glm::ivec2 Input_GetMousePos();
		glm::ivec2 Input_GetMousePosTotal();

		#pragma endregion

		#pragma region Matrix4

		void Matrix4_Inverse(glm::mat4* matrix, glm::mat4* out_Result);
		glm::mat4 Matrix4_Matrix4MulMatrix4(glm::mat4* lhs, glm::mat4* rhs);
		glm::vec4 Matrix4_Matrix4MulVector4(glm::mat4* lhs, glm::vec4* rhs);

		#pragma endregion

		#pragma region Scene

		MonoObject* Scene_InstantiateScript(MonoString* name, MonoReflectionType* scriptType);
		void Scene_CreateEntity(MonoString* name, UUID uuid, UUID* out_UUID);
		void Scene_DestroyEntity(UUID entityHandle);
		MonoObject* Scene_GetScriptObject(UUID scriptEntityHandle);
		bool Scene_IsValidEntityHandle(UUID entityHandle);
		UUID Scene_GetActiveCameraUUID();

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
		UUID SpriteRendererComponent_GetTextureHandle(UUID entityHandle);
		void SpriteRendererComponent_SetTextureHandle(UUID entityHandle, UUID textureHandle);
		float SpriteRendererComponent_GetTilingFactor(UUID entityHandle);
		void SpriteRendererComponent_SetTilingFactor(UUID entityHandle, float tilingFactor);

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
			glm::vec2 Postion;
			float Angle;
		};

		struct Vector2
		{
			float X, Y;
		};

		void* RigidBody2DComponent_GetNativeHandle(UUID owner);
		RigidBody2DTransform RigidBody2DComponent_GetTransform(void* nativeHandle);
		void RigidBody2DComponent_SetTransform(void* nativeHandle, RigidBody2DTransform* transform);
		Vector2 RigidBody2DComponent_GetLocalCenter(void* nativeHandle);
		Vector2 RigidBody2DComponent_GetWorldCenter(void* nativeHandle);
		Vector2 RigidBody2DComponent_GetLinearVelocity(void* nativeHandle);
		void RigidBody2DComponent_SetLinearVelocity(void* nativeHandle, glm::vec2* linearVelocity);
		float RigidBody2DComponent_GetAngularVelocity(void* nativeHandle);
		void RigidBody2DComponent_SetAngularVelocity(void* nativeHandle, float angularVelocity);
		void RigidBody2DComponent_ApplyForce(void* nativeHandle, glm::vec2* force, glm::vec2* point, bool wake);
		void RigidBody2DComponent_ApplyForceToCenter(void* nativeHandle, glm::vec2* force, bool wake);
		void RigidBody2DComponent_ApplyTorque(void* nativeHandle, float torque, bool wake);
		void RigidBody2DComponent_ApplyLinearImpulse(void* nativeHandle, glm::vec2* impulse, glm::vec2* point, bool wake);
		void RigidBody2DComponent_ApplyLinearImpulseToCenter(void* nativeHandle, glm::vec2* impulse, bool wake);
		void RigidBody2DComponent_ApplyAngularImpulse(void* nativeHandle, float impulse, bool wake);

		#pragma endregion

	}

}
