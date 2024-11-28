#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scene/Components.h"
#include "Shark/Input/Input.h"

#include <Coral/String.hpp>
#include <Coral/Array.hpp>

namespace Shark {

	class Entity;

	class ScriptGlue
	{
	public:
		static void Initialize(Coral::ManagedAssembly& assembly);
		static void Shutdown();

	private:
		static void RegisterComponents(Coral::ManagedAssembly& assembly);
		static void RegisterInternalCalls(Coral::ManagedAssembly& assembly);
	};

	namespace InternalCalls {

		#pragma region AssetHandle

		Coral::Bool32 AssetHandle_IsValid(AssetHandle assetHandle);

		#pragma endregion

		#pragma region Application

		uint32_t Application_GetWidth();
		uint32_t Application_GetHeight();

		#pragma endregion

		#pragma region Log

		void Log_LogMessage(LogLevel level, Coral::String message);

		#pragma endregion

		#pragma region Input

		Coral::Bool32 Input_IsKeyStateSet(KeyCode key, KeyState keyState);
		Coral::Bool32 Input_IsMouseStateSet(MouseButton button, MouseState mouseState);
		float Input_GetMouseScroll();
		void Input_GetMousePos(glm::ivec2* out_MousePos);

		#pragma endregion

		#pragma region Matrix4

		void Matrix4_Inverse(glm::mat4* matrix, glm::mat4* out_Result);
		void Matrix4_Matrix4MulMatrix4(glm::mat4* lhs, glm::mat4* rhs, glm::mat4* out_Result);
		void Matrix4_Matrix4MulVector4(glm::mat4* lhs, glm::vec4* rhs, glm::vec4* out_Result);

		#pragma endregion

		#pragma region Scene

		Coral::Bool32 Scene_IsEntityValid(uint64_t entityID);
		uint64_t Scene_CreateEntity(Coral::String name);
		void Scene_DestroyEntity(uint64_t entityID);
		uint64_t Scene_FindEntityByTag(Coral::String Tag);

		#pragma endregion

		#pragma region Entity

		uint64_t Entity_GetParent(uint64_t entityID);
		void Entity_SetParent(uint64_t entityID, uint64_t parentID);
		Coral::Array<uint64_t> Entity_GetChildren(uint64_t entityID);

		Coral::Bool32 Entity_HasComponent(uint64_t entityID, Coral::ReflectionType reflectionType);
		void Entity_AddComponent(uint64_t entityID, Coral::ReflectionType reflectionType);
		void Entity_RemoveComponent(uint64_t entityID, Coral::ReflectionType reflectionType);

		#pragma endregion

		#pragma region TagComponent

		Coral::String TagComponent_GetTag(uint64_t entityID);
		void TagComponent_SetTag(uint64_t entityID, Coral::String tag);

		#pragma endregion

		#pragma region TransformComponent

		void TransformComponent_GetTranslation(uint64_t entityID, glm::vec3* out_Translation);
		void TransformComponent_SetTranslation(uint64_t entityID, glm::vec3* translation);
		
		void TransformComponent_GetRotation(uint64_t entityID, glm::vec3* out_Rotation);
		void TransformComponent_SetRotation(uint64_t entityID, glm::vec3* rotation);
		
		void TransformComponent_GetScale(uint64_t entityID, glm::vec3* out_Scaling);
		void TransformComponent_SetScale(uint64_t entityID, glm::vec3* scaling);

		void TransformComponent_GetLocalTransform(uint64_t entityID, TransformComponent* out_LocalTransform);
		void TransformComponent_SetLocalTransform(uint64_t entityID, TransformComponent* localTransform);
		
		void TransformComponent_GetWorldTransform(uint64_t entityID, TransformComponent* out_WorldTransform);
		void TransformComponent_SetWorldTransform(uint64_t entityID, TransformComponent* worldTransform);

		#pragma endregion

		#pragma region SpriteRendererComponent

		void SpriteRendererComponent_GetColor(uint64_t entityID, glm::vec4* out_Color);
		void SpriteRendererComponent_SetColor(uint64_t entityID, glm::vec4* color);

		AssetHandle SpriteRendererComponent_GetTextureHandle(uint64_t entityID);
		void SpriteRendererComponent_SetTextureHandle(uint64_t entityID, AssetHandle textureHandle);

		void SpriteRendererComponent_GetTilingFactor(uint64_t entityID, glm::vec2* outTilingFactor);
		void SpriteRendererComponent_SetTilingFactor(uint64_t entityID, glm::vec2* tilingFactor);

		#pragma endregion

		#pragma region CricleRendererComponent

		void CircleRendererComponent_GetColor(uint64_t entityID, glm::vec4* out_Color);
		void CircleRendererComponent_SetColor(uint64_t entityID, glm::vec4* color);

		float CircleRendererComponent_GetThickness(uint64_t entityID);
		void CircleRendererComponent_SetThickness(uint64_t entityID, float thickness);

		float CircleRendererComponent_GetFade(uint64_t entityID);
		void CircleRendererComponent_SetFade(uint64_t entityID, float fade);

		#pragma endregion

		#pragma region CameraComponent

		// All calls that modify the component must follow by a call CameraComponent_RecalculateProjectionMatrix
		// otherwise the projection matrix will never reflect the changes
		void CameraComponent_RecalculateProjectionMatrix(uint64_t entityID);

		void CameraComponent_GetProjection(uint64_t entityID, glm::mat4* out_Projection);
		void CameraComponent_SetProjection(uint64_t entityID, glm::mat4* projection);

		Coral::Bool32 CameraComponent_GetIsPerspective(uint64_t entityID);
		void CameraComponent_SetIsPerspective(uint64_t entityID, bool isPerspective);

		void CameraComponent_SetPerspective(uint64_t entityID, float aspectratio, float perspectiveFOV, float clipnear, float clipfar);
		void CameraComponent_SetOrthographic(uint64_t entityID, float aspectratio, float orthographicSize, float clipnear, float clipfar);

		float CameraComponent_GetAspectratio(uint64_t entityID);
		void CameraComponent_SetAspectratio(uint64_t entityID, float aspectratio);

		float CameraComponent_GetPerspectiveFOV(uint64_t entityID);
		void CameraComponent_SetPerspectiveFOV(uint64_t entityID, float fov);

		float CameraComponent_GetOrthographicZoom(uint64_t entityID);
		void CameraComponent_SetOrthographicZoom(uint64_t entityID, float size);

		float CameraComponent_GetPerspectiveNear(uint64_t entityID);
		void CameraComponent_SetPerspectiveNear(uint64_t entityID, float clipnear);

		float CameraComponent_GetPerspectiveFar(uint64_t entityID);
		void CameraComponent_SetPerspectiveFar(uint64_t entityID, float clipfar);

		float CameraComponent_GetOrthographicNear(uint64_t entityID);
		void CameraComponent_SetOrthographicNear(uint64_t entityID, float clipnear);

		float CameraComponent_GetOrthographicFar(uint64_t entityID);
		void CameraComponent_SetOrthographicFar(uint64_t entityID, float clipfar);

		#pragma endregion

		#pragma region Physics2D

		void Physics2D_GetGravity(glm::vec2* out_Gravity);
		void Physics2D_SetGravity(glm::vec2* gravity);

		Coral::Bool32 Physics2D_GetAllowSleep();
		void Physics2D_SetAllowSleep(bool allowSleep);

		#pragma endregion

		#pragma region RigidBody2DComponent

		enum class PhysicsForce2DType
		{
			Force = 0,
			Impulse = 1
		};

		RigidbodyType RigidBody2DComponent_GetBodyType(uint64_t entityID);
		void RigidBody2DComponent_SetBodyType(uint64_t entityID, RigidbodyType bodyType);
		void RigidBody2DComponent_GetPosition(uint64_t entityID, glm::vec2* outPosition);
		void RigidBody2DComponent_SetPosition(uint64_t entityID, glm::vec2* position);
		float RigidBody2DComponent_GetRotation(uint64_t entityID);
		void RigidBody2DComponent_SetRotation(uint64_t entityID, float rotation);
		void RigidBody2DComponent_GetLocalCenter(uint64_t entityID, glm::vec2* out_LocalCenter);
		void RigidBody2DComponent_GetWorldCenter(uint64_t entityID, glm::vec2* out_WorldCenter);
		void RigidBody2DComponent_GetLinearVelocity(uint64_t entityID, glm::vec2* out_LinearVelocity);
		void RigidBody2DComponent_SetLinearVelocity(uint64_t entityID, glm::vec2* linearVelocity);
		float RigidBody2DComponent_GetAngularVelocity(uint64_t entityID);
		void RigidBody2DComponent_SetAngularVelocity(uint64_t entityID, float angularVelocity);
		float RigidBody2DComponent_GetGravityScale(uint64_t entityID);
		void RigidBody2DComponent_SetGravityScale(uint64_t entityID, float gravityScale);
		float RigidBody2DComponent_GetLinearDamping(uint64_t entityID);
		void RigidBody2DComponent_SetLinearDamping(uint64_t entityID, float linearDamping);
		float RigidBody2DComponent_GetAngularDamping(uint64_t entityID);
		void RigidBody2DComponent_SetAngularDamping(uint64_t entityID, float angularDamping);
		Coral::Bool32 RigidBody2DComponent_IsBullet(uint64_t entityID);
		void RigidBody2DComponent_SetBullet(uint64_t entityID, bool bullet);
		Coral::Bool32 RigidBody2DComponent_IsSleepingAllowed(uint64_t entityID);
		void RigidBody2DComponent_SetSleepingAllowed(uint64_t entityID, bool sleepingAllowed);
		Coral::Bool32 RigidBody2DComponent_IsAwake(uint64_t entityID);
		void RigidBody2DComponent_SetAwake(uint64_t entityID, bool awake);
		Coral::Bool32 RigidBody2DComponent_IsEnabled(uint64_t entityID);
		void RigidBody2DComponent_SetEnabled(uint64_t entityID, bool enabled);
		Coral::Bool32 RigidBody2DComponent_IsFixedRotation(uint64_t entityID);
		void RigidBody2DComponent_SetFixedRotation(uint64_t entityID, bool fixedRotation);
		void RigidBody2DComponent_ApplyForce(uint64_t entityID, glm::vec2* force, glm::vec2* point, PhysicsForce2DType forceType);
		void RigidBody2DComponent_ApplyForceToCenter(uint64_t entityID, glm::vec2* force, PhysicsForce2DType forceType);
		void RigidBody2DComponent_ApplyTorque(uint64_t entityID, float torque, PhysicsForce2DType forceType);

		#pragma endregion

		#pragma region BoxCollider2DComponent

		Coral::Bool32 BoxCollider2DComponent_IsSensor(uint64_t entityID);
		void BoxCollider2DComponent_SetSensor(uint64_t entityID, bool sensor);
		float BoxCollider2DComponent_GetDensity(uint64_t entityID);
		void BoxCollider2DComponent_SetDensity(uint64_t entityID, float density);
		float BoxCollider2DComponent_GetFriction(uint64_t entityID);
		void BoxCollider2DComponent_SetFriction(uint64_t entityID, float friction);
		float BoxCollider2DComponent_GetRestitution(uint64_t entityID);
		void BoxCollider2DComponent_SetRestitution(uint64_t entityID, float restitution);
		float BoxCollider2DComponent_GetRestitutionThreshold(uint64_t entityID);
		void BoxCollider2DComponent_SetRestitutionThreshold(uint64_t entityID, float restitutionThreshold);

		void BoxCollider2DComponent_GetSize(uint64_t entityID, glm::vec2* out_Size);
		void BoxCollider2DComponent_SetSize(uint64_t entityID, glm::vec2* size);
		void BoxCollider2DComponent_GetOffset(uint64_t entityID, glm::vec2* out_Offset);
		void BoxCollider2DComponent_SetOffset(uint64_t entityID, glm::vec2* offset);
		float BoxCollider2DComponent_GetRotation(uint64_t entityID);
		void BoxCollider2DComponent_SetRotation(uint64_t entityID, float rotation);

		#pragma endregion

		#pragma region CircleCollider2DComponent

		Coral::Bool32 CircleCollider2DComponent_IsSensor(uint64_t entityID);
		void CircleCollider2DComponent_SetSensor(uint64_t entityID, bool sensor);
		float CircleCollider2DComponent_GetDensity(uint64_t entityID);
		void CircleCollider2DComponent_SetDensity(uint64_t entityID, float density);
		float CircleCollider2DComponent_GetFriction(uint64_t entityID);
		void CircleCollider2DComponent_SetFriction(uint64_t entityID, float friction);
		float CircleCollider2DComponent_GetRestitution(uint64_t entityID);
		void CircleCollider2DComponent_SetRestitution(uint64_t entityID, float restitution);
		float CircleCollider2DComponent_GetRestitutionThreshold(uint64_t entityID);
		void CircleCollider2DComponent_SetRestitutionThreshold(uint64_t entityID, float restitutionThreshold);

		float CircleCollider2DComponent_GetRadius(uint64_t entityID);
		void CircleCollider2DComponent_SetRadius(uint64_t entityID, float Radius);
		void CircleCollider2DComponent_GetOffset(uint64_t entityID, glm::vec2* out_Offsets);
		void CircleCollider2DComponent_SetOffset(uint64_t entityID, glm::vec2* offset);
		float CircleCollider2DComponent_GetRotation(uint64_t entityID);
		void CircleCollider2DComponent_SetRotation(uint64_t entityID, float rotation);

		#pragma endregion

		#pragma region ScriptComponent

		Coral::ManagedObject ScriptComponent_GetInstance(uint64_t entityID);

		#pragma endregion

	}

}
