#include "skpch.h"
#include "MonoGlue.h"

#include "Shark/Core/Log.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"

#include "Shark/Asset/ResourceManager.h"

#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scripting/ScriptManager.h"

#include "Shark/Event/KeyEvent.h"
#include "Shark/Event/MouseEvent.h"

#include "Shark/Utils/MemoryUtils.h"

#include "Shark/Debug/Instrumentor.h"

#include <mono/metadata/appdomain.h>
#include <mono/metadata/reflection.h>

#include <box2d/b2_body.h>
#include <box2d/b2_contact.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>

#define SK_ADD_INTERNAL_CALL(func) mono_add_internal_call("Shark.InternalCalls::" SK_STRINGIFY(func), SK_CONNECT(&InternalCalls::, func));
#define SK_ADD_COMPONENT_BINDING(comp)\
{\
	auto& bindings = s_EntityBindings["Shark." SK_STRINGIFY(comp)];\
	bindings.HasComponent = [](Entity entity) { return entity.HasComponent<comp>(); };\
	bindings.AddComponent = [](Entity entity) { entity.AddComponent<comp>(); };\
}

#if SK_DEBUG
#define SK_GUARD_FAILED_ACTION() SK_CORE_ASSERT(false)
#else
#define SK_GUARD_FAILED_ACTION() (void)0
#endif

#define NULL_ARG
#define SK_INVALID_ENTITY_GUARD(entity, retVal)\
if (!entity)\
{\
	SK_CORE_WARN("InternalCalls: Invalid Entity [{}]", SK_FUNCTION);\
	SK_GUARD_FAILED_ACTION();\
	return retVal;\
}

#define SK_INVALID_NATIVE_HANDLE_GUARD(native_handle, retVal)\
if (!native_handle)\
{\
	SK_CORE_WARN("InternalCalls: Invalid Native Handle [{}]", SK_FUNCTION);\
	SK_GUARD_FAILED_ACTION();\
	return retVal;\
}

#define SK_INVALID_BODY_HANDLE_GUARD(bodyHandle, retVal)\
if (!::Shark::ScriptEngine::GetActiveScene()->GetPhysicsScene().HasBody(bodyHandle))\
{\
	SK_CORE_WARN("InternalCalls: Invalid Body Handle [{}]", SK_FUNCTION);\
	SK_GUARD_FAILED_ACTION();\
	return retVal;\
}

namespace Shark {

	struct EntityBindings
	{
		bool(*HasComponent)(Entity);
		void(*AddComponent)(Entity);
	};

	static std::unordered_map<std::string_view, EntityBindings> s_EntityBindings;

	struct MonoGlueData
	{
		MonoMethod* RaiseOnKeyPressedEvent = nullptr;
		MonoMethod* RaiseOnKeyReleasedEvent = nullptr;
		MonoMethod* RaiseOnMouseMovedEvent = nullptr;
		MonoMethod* RaiseOnMouseButtonPressedEvent = nullptr;
		MonoMethod* RaiseOnMouseButtonReleasedEvent = nullptr;
		MonoMethod* RaiseOnMouseButtonDoubleClickedEvent = nullptr;
		MonoMethod* RaiseOnMouseScrolledEvent = nullptr;
	};
	static Scope<MonoGlueData> s_MonoGlue;

	namespace utils {

		static Entity GetEntityActiveScene(UUID uuid)
		{
			return ScriptEngine::GetActiveScene()->GetEntityByUUID(uuid);
		}

		static RigidBody2DComponent::BodyType b2BodyTypeToSharkBodyType(b2BodyType bodyType)
		{
			switch (bodyType)
			{
				case b2_staticBody:    return RigidBody2DComponent::BodyType::Static;
				case b2_kinematicBody: return RigidBody2DComponent::BodyType::Kinematic;
				case b2_dynamicBody:   return RigidBody2DComponent::BodyType::Dynamic;
			}
			SK_CORE_ASSERT(false, "Invalid Body Type");
			return RigidBody2DComponent::BodyType::Static;
		}

		static b2BodyType SharkBodyTypeTob2BodyType(RigidBody2DComponent::BodyType bodyType)
		{
			switch (bodyType)
			{
				case RigidBody2DComponent::BodyType::Static:    return b2_staticBody;
				case RigidBody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
				case RigidBody2DComponent::BodyType::Dynamic:   return b2_dynamicBody;
			}
			SK_CORE_ASSERT(false, "Invalid Body Type");
			return b2_staticBody;
		}

	}

	void MonoGlue::Init()
	{
		SK_PROFILE_FUNCTION();

		s_MonoGlue = Scope<MonoGlueData>::Create();

		s_MonoGlue->RaiseOnKeyPressedEvent = ScriptEngine::GetMethodCore("Shark.EventHandler:RaiseOnKeyPressed");
		s_MonoGlue->RaiseOnKeyReleasedEvent = ScriptEngine::GetMethodCore("Shark.EventHandler:RaiseOnKeyReleased");
		s_MonoGlue->RaiseOnMouseMovedEvent = ScriptEngine::GetMethodCore("Shark.EventHandler:RaiseOnMouseMoved");
		s_MonoGlue->RaiseOnMouseButtonPressedEvent = ScriptEngine::GetMethodCore("Shark.EventHandler:RaiseOnMouseButtonPressed");
		s_MonoGlue->RaiseOnMouseButtonReleasedEvent = ScriptEngine::GetMethodCore("Shark.EventHandler:RaiseOnMouseButtonReleased");
		s_MonoGlue->RaiseOnMouseButtonDoubleClickedEvent = ScriptEngine::GetMethodCore("Shark.EventHandler:RaiseOnMouseButtonDoubleClicked");
		s_MonoGlue->RaiseOnMouseScrolledEvent = ScriptEngine::GetMethodCore("Shark.EventHandler:RaiseOnMouseScrolled");

		SK_CORE_ASSERT(s_MonoGlue->RaiseOnKeyPressedEvent);
		SK_CORE_ASSERT(s_MonoGlue->RaiseOnKeyReleasedEvent);
		SK_CORE_ASSERT(s_MonoGlue->RaiseOnMouseMovedEvent);
		SK_CORE_ASSERT(s_MonoGlue->RaiseOnMouseButtonPressedEvent);
		SK_CORE_ASSERT(s_MonoGlue->RaiseOnMouseButtonReleasedEvent);
		SK_CORE_ASSERT(s_MonoGlue->RaiseOnMouseButtonDoubleClickedEvent);
		SK_CORE_ASSERT(s_MonoGlue->RaiseOnMouseScrolledEvent);

		RegisterComponents();
		RegsiterInternalCalls();


	}

	void MonoGlue::Shutdown()
	{
		s_MonoGlue = nullptr;
		s_EntityBindings.clear();
	}

	void MonoGlue::CallCollishionBegin(Entity entityA, Entity entityB)
	{
		SK_PROFILE_FUNCTION();

		if (!s_MonoGlue)
			return;

		const UUID uuidA = entityA.GetUUID();
		const UUID uuidB = entityB.GetUUID();

		const bool AIsScript = ScriptManager::Contains(uuidA);
		const bool BIsScript = ScriptManager::Contains(uuidB);

		if (AIsScript)
		{
			auto& script = ScriptManager::GetScript(uuidA);
			script.OnCollishionBegin(uuidB, BIsScript);
		}
		
		if (BIsScript)
		{
			auto& script = ScriptManager::GetScript(uuidB);
			script.OnCollishionBegin(uuidA, AIsScript);
		}

	}

	void MonoGlue::CallCollishionEnd(Entity entityA, Entity entityB)
	{
		SK_PROFILE_FUNCTION();

		if (!s_MonoGlue)
			return;

		const UUID uuidA = entityA.GetUUID();
		const UUID uuidB = entityB.GetUUID();

		const bool AIsScript = ScriptManager::Contains(uuidA);
		const bool BIsScript = ScriptManager::Contains(uuidB);

		if (AIsScript)
		{
			auto& script = ScriptManager::GetScript(uuidA);
			script.OnCollishionEnd(uuidB, BIsScript);
		}

		if (BIsScript)
		{
			auto& script = ScriptManager::GetScript(uuidB);
			script.OnCollishionEnd(uuidA, AIsScript);
		}

	}

	void MonoGlue::OnEvent(Event& event)
	{
		if (!s_MonoGlue)
			return;

		EventDispacher dispacher(event);
		dispacher.DispachEventAlways<KeyPressedEvent>([](auto& e) { ScriptEngine::InvokeMethod(s_MonoGlue->RaiseOnKeyPressedEvent, nullptr, e.GetKeyCode(), e.IsRepeat()); return false; });
		dispacher.DispachEventAlways<KeyReleasedEvent>([](auto& e) { ScriptEngine::InvokeMethod(s_MonoGlue->RaiseOnKeyReleasedEvent, nullptr, e.GetKeyCode()); return false; });

		dispacher.DispachEventAlways<MouseMovedEvent>([](auto& e) { ScriptEngine::InvokeMethod(s_MonoGlue->RaiseOnMouseMovedEvent, nullptr, e.GetMousePos()); return false; });
		dispacher.DispachEventAlways<MouseButtonPressedEvent>([](auto& e) { ScriptEngine::InvokeMethod(s_MonoGlue->RaiseOnMouseButtonPressedEvent, nullptr, e.GetButton(), e.GetMousePos()); return false; });
		dispacher.DispachEventAlways<MouseButtonReleasedEvent>([](auto& e) { ScriptEngine::InvokeMethod(s_MonoGlue->RaiseOnMouseButtonReleasedEvent, nullptr, e.GetButton(), e.GetMousePos()); return false; });
		dispacher.DispachEventAlways<MouseButtonDoubleClickedEvent>([](auto& e) { ScriptEngine::InvokeMethod(s_MonoGlue->RaiseOnMouseButtonDoubleClickedEvent, nullptr, e.GetButton(), e.GetMousePos()); return false; });
		dispacher.DispachEventAlways<MouseScrolledEvent>([](auto& e) { ScriptEngine::InvokeMethod(s_MonoGlue->RaiseOnMouseScrolledEvent, nullptr, e.GetDelta(), e.GetMousePos()); return false; });
	}

	void MonoGlue::RegisterComponents()
	{
		SK_PROFILE_FUNCTION();

		SK_ADD_COMPONENT_BINDING(IDComponent);
		SK_ADD_COMPONENT_BINDING(TagComponent);
		SK_ADD_COMPONENT_BINDING(TransformComponent);
		SK_ADD_COMPONENT_BINDING(SpriteRendererComponent);
		SK_ADD_COMPONENT_BINDING(CircleRendererComponent);
		SK_ADD_COMPONENT_BINDING(CameraComponent);
		SK_ADD_COMPONENT_BINDING(RigidBody2DComponent);
		SK_ADD_COMPONENT_BINDING(BoxCollider2DComponent);
		SK_ADD_COMPONENT_BINDING(CircleCollider2DComponent);
	}

	void MonoGlue::RegsiterInternalCalls()
	{
		SK_PROFILE_FUNCTION();

		SK_ADD_INTERNAL_CALL(Log_LogLevel);
		SK_ADD_INTERNAL_CALL(UUID_Generate);

		SK_ADD_INTERNAL_CALL(Input_KeyPressed);
		SK_ADD_INTERNAL_CALL(Input_MouseButtonPressed);
		SK_ADD_INTERNAL_CALL(Input_GetMousePos);
		SK_ADD_INTERNAL_CALL(Input_GetMousePosGlobal);

		SK_ADD_INTERNAL_CALL(Matrix4_Inverse);
		SK_ADD_INTERNAL_CALL(Matrix4_Matrix4MulMatrix4);
		SK_ADD_INTERNAL_CALL(Matrix4_Matrix4MulVector4);

		SK_ADD_INTERNAL_CALL(Scene_InstantiateScript);
		SK_ADD_INTERNAL_CALL(Scene_CreateEntity);
		SK_ADD_INTERNAL_CALL(Scene_DestroyEntity);
		SK_ADD_INTERNAL_CALL(Scene_CloneEntity);
		SK_ADD_INTERNAL_CALL(Scene_GetScriptObject);
		SK_ADD_INTERNAL_CALL(Scene_IsValidEntityHandle);
		SK_ADD_INTERNAL_CALL(Scene_GetActiveCameraUUID);
		SK_ADD_INTERNAL_CALL(Scene_GetUUIDFromTag);
		SK_ADD_INTERNAL_CALL(Scene_GetViewportBounds);

		SK_ADD_INTERNAL_CALL(Entity_GetName);
		SK_ADD_INTERNAL_CALL(Entity_SetName);
		SK_ADD_INTERNAL_CALL(Entity_HasComponent);
		SK_ADD_INTERNAL_CALL(Entity_AddComponent);

		SK_ADD_INTERNAL_CALL(TransformComponent_GetTranslation);
		SK_ADD_INTERNAL_CALL(TransformComponent_SetTranslation);
		SK_ADD_INTERNAL_CALL(TransformComponent_GetRotation);
		SK_ADD_INTERNAL_CALL(TransformComponent_SetRotation);
		SK_ADD_INTERNAL_CALL(TransformComponent_GetScaling);
		SK_ADD_INTERNAL_CALL(TransformComponent_SetScaling);

		SK_ADD_INTERNAL_CALL(SpriteRendererComponent_GetColor);
		SK_ADD_INTERNAL_CALL(SpriteRendererComponent_SetColor);
		SK_ADD_INTERNAL_CALL(SpriteRendererComponent_SetTextureHandle);
		SK_ADD_INTERNAL_CALL(SpriteRendererComponent_GetTextureHandle);
		SK_ADD_INTERNAL_CALL(SpriteRendererComponent_GetTilingFactor);
		SK_ADD_INTERNAL_CALL(SpriteRendererComponent_SetTilingFactor);

		SK_ADD_INTERNAL_CALL(CameraComponent_GetProjection);
		SK_ADD_INTERNAL_CALL(CameraComponent_SetProjection);
		SK_ADD_INTERNAL_CALL(CameraComponent_GetProjectionType);
		SK_ADD_INTERNAL_CALL(CameraComponent_SetProjectionType);
		SK_ADD_INTERNAL_CALL(CameraComponent_SetPerspective);
		SK_ADD_INTERNAL_CALL(CameraComponent_SetOrthographic);
		SK_ADD_INTERNAL_CALL(CameraComponent_GetAspectratio);
		SK_ADD_INTERNAL_CALL(CameraComponent_SetAspectratio);
		SK_ADD_INTERNAL_CALL(CameraComponent_GetPerspectiveFOV);
		SK_ADD_INTERNAL_CALL(CameraComponent_SetPerspectiveFOV);
		SK_ADD_INTERNAL_CALL(CameraComponent_GetPerspectiveNear);
		SK_ADD_INTERNAL_CALL(CameraComponent_SetPerspectiveNear);
		SK_ADD_INTERNAL_CALL(CameraComponent_GetPerspectiveFar);
		SK_ADD_INTERNAL_CALL(CameraComponent_SetPerspectiveFar);
		SK_ADD_INTERNAL_CALL(CameraComponent_GetOrthographicZoom);
		SK_ADD_INTERNAL_CALL(CameraComponent_SetOrthographicZoom);
		SK_ADD_INTERNAL_CALL(CameraComponent_GetOrthographicNear);
		SK_ADD_INTERNAL_CALL(CameraComponent_SetOrthographicNear);
		SK_ADD_INTERNAL_CALL(CameraComponent_GetOrthographicFar);
		SK_ADD_INTERNAL_CALL(CameraComponent_SetOrthographicFar);

		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_GetNativeHandle);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_GetTransform);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_SetTransform);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_SetPosition);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_SetRotation);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_GetLocalCenter);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_GetWorldCenter);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_GetLinearVelocity);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_SetLinearVelocity);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_GetAngularVelocity);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_SetAngularVelocity);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_ApplyForce);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_ApplyForceToCenter);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_ApplyTorque);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_GetGravityScale);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_SetGravityScale);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_GetLinearDamping);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_SetLinearDamping);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_GetAngularDamping);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_SetAngularDamping);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_IsBullet);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_SetBullet);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_IsSleepingAllowed);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_SetSleepingAllowed);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_IsAwake);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_SetAwake);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_IsEnabled);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_SetEnabled);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_IsFixedRotation);
		SK_ADD_INTERNAL_CALL(RigidBody2DComponent_SetFixedRotation);

		SK_ADD_INTERNAL_CALL(PhysicsCollider2D_SetSensor);
		SK_ADD_INTERNAL_CALL(PhysicsCollider2D_IsSensor);
		SK_ADD_INTERNAL_CALL(PhysicsCollider2D_SetDensity);
		SK_ADD_INTERNAL_CALL(PhysicsCollider2D_GetDensity);
		SK_ADD_INTERNAL_CALL(PhysicsCollider2D_SetFriction);
		SK_ADD_INTERNAL_CALL(PhysicsCollider2D_GetFriction);
		SK_ADD_INTERNAL_CALL(PhysicsCollider2D_SetRestitution);
		SK_ADD_INTERNAL_CALL(PhysicsCollider2D_GetRestitution);
		SK_ADD_INTERNAL_CALL(PhysicsCollider2D_SetRestitutionThreshold);
		SK_ADD_INTERNAL_CALL(PhysicsCollider2D_GetRestitutionThreshold);

		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetNativeHandle);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetSize);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetSize);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetOffset);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetOffset);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetRotation);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetRotation);

		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetNativeHandle);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetRadius);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetRadius);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetOffset);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetOffset);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetRotation);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetRotation);

		SK_ADD_INTERNAL_CALL(ResourceManager_GetAssetHandleFromFilePath);

	}

	namespace InternalCalls {

		#pragma region Log

		void Log_LogLevel(LogLevel level, MonoString* message)
		{
			SK_PROFILE_FUNCTION();

			char* msg = mono_string_to_utf8(message);
			switch (level)
			{
				case LogLevel::Trace: SK_TRACE(msg); break;
				case LogLevel::Debug: Shark::Log::GetClientLogger()->debug(msg); break;
				case LogLevel::Info: SK_INFO(msg); break;
				case LogLevel::Warn: SK_WARN(msg); break;
				case LogLevel::Error: SK_ERROR(msg); break;
				case LogLevel::Critical: SK_CRITICAL(msg); break;
			}
		}

		#pragma endregion

		#pragma region UUID

		UUID UUID_Generate()
		{
			SK_PROFILE_FUNCTION();

			return UUID::Generate();
		}

		#pragma endregion

		#pragma region Input

		bool Input_KeyPressed(KeyCode key)
		{
			return Input::KeyPressed(key);
		}

		bool Input_MouseButtonPressed(MouseButton::Type button)
		{
			return Input::MousePressed(button);
		}

		Vector2i Input_GetMousePos()
		{
			auto p = Input::MousePos();
			return { p.x, p.y };
		}

		Vector2i Input_GetMousePosGlobal()
		{
			auto p = Input::GlobalMousePos();
			return { p.x, p.y };
		}

		#pragma endregion

		#pragma region Matrix4

		void Matrix4_Inverse(glm::mat4* matrix, glm::mat4* out_Result)
		{
			SK_PROFILE_FUNCTION();

			*out_Result = glm::inverse(*matrix);
		}

		glm::mat4 Matrix4_Matrix4MulMatrix4(glm::mat4* lhs, glm::mat4* rhs)
		{
			SK_PROFILE_FUNCTION();

			return *lhs * *rhs;
		}

		glm::vec4 Matrix4_Matrix4MulVector4(glm::mat4* lhs, glm::vec4* rhs)
		{
			SK_PROFILE_FUNCTION();

			return *lhs * *rhs;
		}

		#pragma endregion

		#pragma region Scene

		MonoObject* Scene_InstantiateScript(MonoReflectionType* scriptType, MonoString* name)
		{
			SK_PROFILE_FUNCTION();

			MonoType* monoType = mono_reflection_type_get_type(scriptType);
			const char* scriptTypeName = mono_type_get_name(monoType);

			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			Entity newEntity = scene->CreateEntity(mono_string_to_utf8(name));
			auto& comp = newEntity.AddComponent<ScriptComponent>();
			comp.ScriptName = scriptTypeName;
			comp.ScriptModuleFound = true;

			if (ScriptManager::Instantiate(newEntity, true))
			{
				auto& script = ScriptManager::GetScript(newEntity.GetUUID());
				return script.GetObject();
			}
			return nullptr;
		} 

		void Scene_CreateEntity(MonoString* name, UUID uuid, UUID* out_UUID)
		{
			SK_PROFILE_FUNCTION();

			Ref<Scene> scene = ScriptEngine::GetActiveScene();

			const char* entityName = mono_string_to_utf8(name);
			Entity entity = scene->CreateEntityWithUUID(uuid.IsValid() ? uuid : UUID::Generate(), entityName ? entityName : std::string{});
			*out_UUID = entity.GetUUID();
		}

		void Scene_DestroyEntity(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			Entity entity = scene->GetEntityByUUID(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			scene->DestroyEntity(entity);
		}

		void Scene_CloneEntity(UUID entityHandle, UUID* out_UUID)
		{
			SK_PROFILE_FUNCTION();

			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			Entity entity = scene->GetEntityByUUID(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			Entity clonedEntity = scene->CloneEntity(entity);
			*out_UUID = clonedEntity.GetUUID();
		}

		MonoObject* Scene_GetScriptObject(UUID scriptEntityHandle)
		{
			SK_PROFILE_FUNCTION();

			if (!ScriptManager::Contains(scriptEntityHandle))
				return nullptr;

			auto& script = ScriptManager::GetScript(scriptEntityHandle);
			return script.GetObject();
		}

		bool Scene_IsValidEntityHandle(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			return scene->GetEntityByUUID(entityHandle);
		}

		void Scene_GetActiveCameraUUID(UUID* out_UUID)
		{
			SK_PROFILE_FUNCTION();

			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			*out_UUID = scene->GetActiveCameraUUID();
		}

		void Scene_GetUUIDFromTag(MonoString* tag, UUID* out_UUID)
		{
			SK_PROFILE_FUNCTION();

			const char* cStr = mono_string_to_utf8(tag);

			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			auto view = scene->GetAllEntitysWith<TagComponent>();
			for (auto entityID : view)
			{
				Entity entity{ entityID, scene };
				if (entity.GetName() == cStr)
				{
					*out_UUID = entity.GetUUID();
					return;
				}
			}
		}

		Bounds2i Scene_GetViewportBounds()
		{
			SK_PROFILE_FUNCTION();

			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			return scene->GetViewportBounds();
		}

#pragma endregion

		#pragma region Entity

		MonoString* Entity_GetName(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, nullptr);
			const auto& name = entity.GetName();
			return mono_string_new(mono_domain_get(), name.c_str());
		}

		void Entity_SetName(UUID entityHandle, MonoString* name)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& comp = entity.GetComponent<TagComponent>();
			comp.Tag = mono_string_to_utf8(name);
		}

		bool Entity_HasComponent(UUID entityHandle, MonoReflectionType* type)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, false);

			MonoType* t = mono_reflection_type_get_type(type);
			const char* typeName = mono_type_get_name(t);
			const auto& bindings = s_EntityBindings.at(typeName);
			return bindings.HasComponent(entity);
		}

		void Entity_AddComponent(UUID entityHandle, MonoReflectionType* type)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);

			MonoType* t = mono_reflection_type_get_type(type);
			const char* typeName = mono_type_get_name(t);
			const auto& bindings = s_EntityBindings.at(typeName);
			bindings.AddComponent(entity);
		}

#pragma endregion

		#pragma region TransformComponent

		glm::vec3 TransformComponent_GetTranslation(UUID uuid)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(uuid);
			SK_INVALID_ENTITY_GUARD(entity, glm::vec3());
			auto& transform = entity.GetTransform();
			return transform.Position;
		}

		void TransformComponent_SetTranslation(UUID uuid, glm::vec3 translation)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(uuid);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& transform = entity.GetTransform();
			transform.Position = translation;
		}

		glm::vec3 TransformComponent_GetRotation(UUID uuid)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(uuid);
			SK_INVALID_ENTITY_GUARD(entity, glm::vec3());
			auto& transform = entity.GetTransform();
			return transform.Rotation;
		}

		void TransformComponent_SetRotation(UUID uuid, glm::vec3 rotation)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(uuid);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& transform = entity.GetTransform();
			transform.Rotation = rotation;
		}

		glm::vec3 TransformComponent_GetScaling(UUID uuid)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(uuid);
			SK_INVALID_ENTITY_GUARD(entity, glm::vec3());
			auto& transform = entity.GetTransform();
			return transform.Scaling;
		}

		void TransformComponent_SetScaling(UUID uuid, glm::vec3 scaling)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(uuid);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& transform = entity.GetTransform();
			transform.Scaling = scaling;
		}

#pragma endregion

		#pragma region SpriteRendererComponent

		glm::vec4 SpriteRendererComponent_GetColor(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, glm::vec4());
			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			return spriteRenderer.Color;
		}

		void SpriteRendererComponent_SetColor(UUID entityHandle, glm::vec4 color)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			spriteRenderer.Color = color;
		}

		AssetHandle SpriteRendererComponent_GetTextureHandle(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, UUID::Null());
			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			return spriteRenderer.TextureHandle;
		}

		void SpriteRendererComponent_SetTextureHandle(UUID entityHandle, AssetHandle textureHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			spriteRenderer.TextureHandle = textureHandle;
		}

		float SpriteRendererComponent_GetTilingFactor(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, float());
			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			return spriteRenderer.TilingFactor;
		}

		void SpriteRendererComponent_SetTilingFactor(UUID entityHandle, float tilingFactor)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			spriteRenderer.TilingFactor = tilingFactor;
		}

		#pragma endregion

		#pragma region CricleRendererComponent

		glm::vec4 CircleRendererComponent_GetColor(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, glm::vec4());
			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			return circleRenderer.Color;
		}

		void CircleRendererComponent_SetColor(UUID entityHandle, glm::vec4 color)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			circleRenderer.Color = color;
		}

		float CircleRendererComponent_GetThickness(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, {});
			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			return circleRenderer.Thickness;
		}

		void CircleRendererComponent_SetThickness(UUID entityHandle, float thickness)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			circleRenderer.Thickness = thickness;
		}

		float CircleRendererComponent_GetFade(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, {});
			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			return circleRenderer.Fade;
		}

		void CircleRendererComponent_SetFade(UUID entityHandle, float fade)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			circleRenderer.Fade = fade;
		}

		#pragma endregion

		#pragma region CameraComponent

		glm::mat4 CameraComponent_GetProjection(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, {});
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetProjection();
		}

		void CameraComponent_SetProjection(UUID entityHandle, glm::mat4 projection)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetProjection(projection);
		}

		SceneCamera::Projection CameraComponent_GetProjectionType(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, {});
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetProjectionType();
		}

		void CameraComponent_SetProjectionType(UUID entityHandle, SceneCamera::Projection projectionType)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetProjectionType(projectionType);
		}
		
		void CameraComponent_SetPerspective(UUID entityHandle, float aspectratio, float fov, float clipnear, float clipfar)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspective(aspectratio, fov, clipnear, clipfar);
		}

		void CameraComponent_SetOrthographic(UUID entityHandle, float aspectratio, float zoom, float clipnear, float clipfar)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographic(aspectratio, zoom, clipnear, clipfar);
		}

		float CameraComponent_GetAspectratio(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, {});
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetAspectratio();
		}

		void CameraComponent_SetAspectratio(UUID entityHandle, float aspectratio)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetAspectratio(aspectratio);
		}

		float CameraComponent_GetPerspectiveFOV(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, {});
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetPerspectiveFOV();
		}

		void CameraComponent_SetPerspectiveFOV(UUID entityHandle, float fov)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspectiveFOV(fov);
		}

		float CameraComponent_GetPerspectiveNear(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, {});
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetPerspectiveNear();
		}

		void CameraComponent_SetPerspectiveNear(UUID entityHandle, float clipnear)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspectiveNear(clipnear);
		}

		float CameraComponent_GetPerspectiveFar(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, {});
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetPerspectiveFar();
		}

		void CameraComponent_SetPerspectiveFar(UUID entityHandle, float clipfar)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspectiveFar(clipfar);
		}

		float CameraComponent_GetOrthographicZoom(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, {});
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetOrthographicZoom();
		}

		void CameraComponent_SetOrthographicZoom(UUID entityHandle, float zoom)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographicZoom(zoom);
		}

		float CameraComponent_GetOrthographicNear(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, {});
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetOrthographicNear();
		}

		void CameraComponent_SetOrthographicNear(UUID entityHandle, float clipnear)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographicNear(clipnear);
		}

		float CameraComponent_GetOrthographicFar(UUID entityHandle)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, {});
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetOrthographicFar();
		}
		
		void CameraComponent_SetOrthographicFar(UUID entityHandle, float clipfar)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(entityHandle);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographicFar(clipfar);
		}

		#pragma endregion

		#pragma region RigidBody2DComponent

		void* RigidBody2DComponent_GetNativeHandle(UUID owner)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, nullptr);
			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			return comp.RuntimeBody;
		}

		RigidBody2DComponent::BodyType RigidBody2DComponent_GetBodyType(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			return utils::b2BodyTypeToSharkBodyType(body->GetType());
		}

		void RigidBody2DComponent_SetBodyType(void* nativeHandle, RigidBody2DComponent::BodyType bodyType)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			body->SetType(utils::SharkBodyTypeTob2BodyType(bodyType));
		}

		RigidBody2DTransform RigidBody2DComponent_GetTransform(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			const auto& tf = body->GetTransform();
			
			RigidBody2DTransform transform;
			transform.Position = { tf.p.x, tf.p.y };
			transform.Angle = tf.q.GetAngle();
			return transform;
		}

		void RigidBody2DComponent_SetTransform(void* nativeHandle, RigidBody2DTransform* transform)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);
			
			const b2Vec2 pos = { transform->Position.x, transform->Position.y };
			body->SetTransform(pos, transform->Angle);
		}

		void RigidBody2DComponent_SetPosition(void* nativeHandle, glm::vec2 position)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			body->SetTransform({ position.x, position.y }, body->GetAngle());
		}

		void RigidBody2DComponent_SetRotation(void* nativeHandle, float rotation)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			body->SetTransform(body->GetPosition(), rotation);
		}

		Vector2 RigidBody2DComponent_GetLocalCenter(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			const b2Vec2 lc = body->GetLocalCenter();
			return { lc.x, lc.y };
		}

		Vector2 RigidBody2DComponent_GetWorldCenter(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			const b2Vec2 wc = body->GetWorldCenter();
			return { wc.x, wc.y };
		}

		Vector2 RigidBody2DComponent_GetLinearVelocity(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			const b2Body* body = (const b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			const b2Vec2& lv = body->GetLinearVelocity();
			return { lv.x, lv.y };
		}

		void RigidBody2DComponent_SetLinearVelocity(void* nativeHandle, glm::vec2* linearVelocity)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			body->SetLinearVelocity({ linearVelocity->x, linearVelocity->y });
		}

		float RigidBody2DComponent_GetAngularVelocity(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			return body->GetAngularVelocity();
		}

		void RigidBody2DComponent_SetAngularVelocity(void* nativeHandle, float angularVelocity)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			body->SetAngularVelocity(angularVelocity);
		}

		void RigidBody2DComponent_ApplyForce(void* nativeHandle, glm::vec2* force, glm::vec2* point, PhysicsForce2DType forceType)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			if (forceType == PhysicsForce2DType::Force)
				body->ApplyForce({ force->x, force->y }, { point->x, point->y }, true);
			else
				body->ApplyLinearImpulse({ force->x, force->y }, { point->x, point->y }, true);
		}

		void RigidBody2DComponent_ApplyForceToCenter(void* nativeHandle, glm::vec2* force, PhysicsForce2DType forceType)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			if (forceType == PhysicsForce2DType::Force)
				body->ApplyForceToCenter({ force->x, force->y }, true);
			else
				body->ApplyLinearImpulseToCenter({ force->x, force->y }, true);
		}

		void RigidBody2DComponent_ApplyTorque(void* nativeHandle, float torque, PhysicsForce2DType forceType)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			if (forceType == PhysicsForce2DType::Force)
				body->ApplyTorque(torque, true);
			else
				body->ApplyAngularImpulse(torque, true);
		}

		float RigidBody2DComponent_GetGravityScale(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			return body->GetGravityScale();
		}

		void RigidBody2DComponent_SetGravityScale(void* nativeHandle, float gravityScale)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			body->SetGravityScale(gravityScale);
		}

		float RigidBody2DComponent_GetLinearDamping(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			return body->GetLinearDamping();
		}

		void RigidBody2DComponent_SetLinearDamping(void* nativeHandle, float linearDamping)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			body->SetLinearDamping(linearDamping);
		}

		float RigidBody2DComponent_GetAngularDamping(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			return body->GetAngularDamping();
		}

		void RigidBody2DComponent_SetAngularDamping(void* nativeHandle, float angularDamping)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			body->SetAngularDamping(angularDamping);
		}

		bool RigidBody2DComponent_IsBullet(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			return body->IsBullet();
		}

		void RigidBody2DComponent_SetBullet(void* nativeHandle, bool bullet)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			body->SetBullet(bullet);
		}

		bool RigidBody2DComponent_IsSleepingAllowed(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			return body->IsSleepingAllowed();
		}

		void RigidBody2DComponent_SetSleepingAllowed(void* nativeHandle, bool sleepingAllowed)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			body->SetSleepingAllowed(sleepingAllowed);
		}

		bool RigidBody2DComponent_IsAwake(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			return body->IsAwake();
		}

		void RigidBody2DComponent_SetAwake(void* nativeHandle, bool awake)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			body->SetAwake(awake);
		}

		bool RigidBody2DComponent_IsEnabled(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			return body->IsEnabled();
		}

		void RigidBody2DComponent_SetEnabled(void* nativeHandle, bool enabled)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			body->SetEnabled(enabled);
		}

		bool RigidBody2DComponent_IsFixedRotation(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, {});

			return body->IsFixedRotation();
		}

		void RigidBody2DComponent_SetFixedRotation(void* nativeHandle, bool fixedRotation)
		{
			SK_PROFILE_FUNCTION();

			b2Body* body = (b2Body*)nativeHandle;
			SK_INVALID_BODY_HANDLE_GUARD(body, NULL_ARG);

			body->SetFixedRotation(fixedRotation);
		}

		#pragma endregion

		#pragma region PhysicsCollider2D

		void PhysicsCollider2D_SetSensor(void* nativeHandle, bool sensor)
		{
			SK_PROFILE_FUNCTION();

			b2Fixture* fixture = (b2Fixture*)nativeHandle;
			fixture->SetSensor(sensor);
		}

		bool PhysicsCollider2D_IsSensor(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Fixture* fixture = (b2Fixture*)nativeHandle;
			return fixture->IsSensor();
		}

		void PhysicsCollider2D_SetDensity(void* nativeHandle, float density)
		{
			SK_PROFILE_FUNCTION();

			b2Fixture* fixture = (b2Fixture*)nativeHandle;
			fixture->SetDensity(density);
		}

		float PhysicsCollider2D_GetDensity(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Fixture* fixture = (b2Fixture*)nativeHandle;
			return fixture->GetDensity();
		}

		void PhysicsCollider2D_SetFriction(void* nativeHandle, float friction)
		{
			SK_PROFILE_FUNCTION();

			b2Fixture* fixture = (b2Fixture*)nativeHandle;
			fixture->SetFriction(friction);
		}

		float PhysicsCollider2D_GetFriction(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Fixture* fixture = (b2Fixture*)nativeHandle;
			return fixture->GetFriction();
		}

		void PhysicsCollider2D_SetRestitution(void* nativeHandle, float restitution)
		{
			SK_PROFILE_FUNCTION();

			b2Fixture* fixture = (b2Fixture*)nativeHandle;
			fixture->SetRestitution(restitution);
		}

		float PhysicsCollider2D_GetRestitution(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Fixture* fixture = (b2Fixture*)nativeHandle;
			return fixture->GetRestitution();
		}

		void PhysicsCollider2D_SetRestitutionThreshold(void* nativeHandle, float restitutionThreshold)
		{
			SK_PROFILE_FUNCTION();

			b2Fixture* fixture = (b2Fixture*)nativeHandle;
			fixture->SetRestitutionThreshold(restitutionThreshold);
		}

		float PhysicsCollider2D_GetRestitutionThreshold(void* nativeHandle)
		{
			SK_PROFILE_FUNCTION();

			b2Fixture* fixture = (b2Fixture*)nativeHandle;
			return fixture->GetRestitutionThreshold();
		}

		#pragma endregion

		#pragma region BoxCollider2DComponent

		void* BoxCollider2DComponent_GetNativeHandle(UUID owner)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, nullptr);
			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			return comp.RuntimeCollider;
		}

		glm::vec2 BoxCollider2DComponent_GetSize(UUID owner)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, {});

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			return comp.Size;
		}

		void BoxCollider2DComponent_SetSize(UUID owner, glm::vec2 size)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			const auto& transform = entity.GetTransform();
			comp.Size = size;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_polygon);
			b2PolygonShape* shape = (b2PolygonShape*)comp.RuntimeCollider->GetShape();
			shape->SetAsBox(comp.Size.x * transform.Scaling.x, comp.Size.y * transform.Scaling.y, { comp.Offset.x, comp.Offset.y }, comp.Rotation);
		}

		glm::vec2 BoxCollider2DComponent_GetOffset(UUID owner)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, {});

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			return comp.Offset;
		}

		void BoxCollider2DComponent_SetOffset(UUID owner, glm::vec2 offset)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			const auto& transform = entity.GetTransform();
			comp.Offset = offset;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_polygon);
			b2PolygonShape* shape = (b2PolygonShape*)comp.RuntimeCollider->GetShape();
			shape->SetAsBox(comp.Size.x * transform.Scaling.x, comp.Size.y * transform.Scaling.y, { comp.Offset.x, comp.Offset.y }, comp.Rotation);
		}

		float BoxCollider2DComponent_GetRotation(UUID owner)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, {});

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			return comp.Rotation;
		}

		void BoxCollider2DComponent_SetRotation(UUID owner, float rotation)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			const auto& transform = entity.GetTransform();
			comp.Rotation = rotation;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_polygon);
			b2PolygonShape* shape = (b2PolygonShape*)comp.RuntimeCollider->GetShape();
			shape->SetAsBox(comp.Size.x * transform.Scaling.x, comp.Size.y * transform.Scaling.y, { comp.Offset.x, comp.Offset.y }, comp.Rotation);
		}

		#pragma endregion

		#pragma region CircleCollider2DComponent

		void* CircleCollider2DComponent_GetNativeHandle(UUID owner)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, nullptr);
			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			return comp.RuntimeCollider;
		}

		float CircleCollider2DComponent_GetRadius(UUID owner)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, {});

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			return comp.Radius;
		}

		void CircleCollider2DComponent_SetRadius(UUID owner, float Radius)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			const auto& transform = entity.GetTransform();
			comp.Radius = Radius;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_circle);
			b2CircleShape* shape = (b2CircleShape*)comp.RuntimeCollider->GetShape();
			shape->m_radius = comp.Radius * transform.Scaling.x;
			shape->m_p = { comp.Offset.x, comp.Offset.y };
		}

		glm::vec2 CircleCollider2DComponent_GetOffset(UUID owner)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, {});

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			return comp.Offset;
		}

		void CircleCollider2DComponent_SetOffset(UUID owner, glm::vec2 offset)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			const auto& transform = entity.GetTransform();
			comp.Offset = offset;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_circle);
			b2CircleShape* shape = (b2CircleShape*)comp.RuntimeCollider->GetShape();
			shape->m_radius = comp.Radius * transform.Scaling.x;
			shape->m_p = { comp.Offset.x, comp.Offset.y };
		}

		float CircleCollider2DComponent_GetRotation(UUID owner)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, {});

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			return comp.Rotation;
		}

		void CircleCollider2DComponent_SetRotation(UUID owner, float rotation)
		{
			SK_PROFILE_FUNCTION();

			Entity entity = utils::GetEntityActiveScene(owner);
			SK_INVALID_ENTITY_GUARD(entity, NULL_ARG);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			const auto& transform = entity.GetTransform();
			comp.Rotation = rotation;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_circle);
			b2CircleShape* shape = (b2CircleShape*)comp.RuntimeCollider->GetShape();
			shape->m_radius = comp.Radius * transform.Scaling.x;
			shape->m_p = { comp.Offset.x, comp.Offset.y };
		}

		#pragma endregion

		#pragma region ResoureManager

		void ResourceManager_GetAssetHandleFromFilePath(MonoString* filePath, AssetHandle* out_AssetHandle)
		{
			SK_PROFILE_FUNCTION();

			const wchar_t* cFilePath = mono_string_to_utf16(filePath);
			*out_AssetHandle = ResourceManager::GetAssetHandleFromFilePath(cFilePath);
		}

		#pragma endregion

	}

}
