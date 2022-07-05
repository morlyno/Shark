#include "skpch.h"
#include "ScriptGlue.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Log.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"

#include "Shark/Asset/ResourceManager.h"

#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scripting/GCManager.h"

#include "Shark/Event/KeyEvent.h"
#include "Shark/Event/MouseEvent.h"

#include "Shark/Utils/MemoryUtils.h"

#include "Shark/Debug/Instrumentor.h"


#include <mono/metadata/appdomain.h>
#include <mono/metadata/object.h>
#include <mono/metadata/reflection.h>
#include <mono/metadata/exception.h>
#include <mono/metadata/debug-helpers.h>

#include <box2d/b2_body.h>
#include <box2d/b2_contact.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>

#include <imgui_internal.h>

#include <glm/gtx/common.hpp>

#define SK_ADD_INTERNAL_CALL(func) mono_add_internal_call("Shark.InternalCalls::" SK_STRINGIFY(func), SK_CONNECT(&InternalCalls::, func));
#define SK_ADD_COMPONENT_BINDING(comp)\
{\
	auto& bindings = s_EntityBindings["Shark." SK_STRINGIFY(comp)];\
	bindings.HasComponent = [](Entity entity) { return entity.AllOf<comp>(); };\
	bindings.AddComponent = [](Entity entity) { entity.AddComponent<comp>(); };\
	bindings.RemoveComponent = [](Entity entity) { entity.RemoveComponent<comp>(); };\
}

#if SK_DEBUG
#define SK_GUARD_FAILED_ACTION() SK_DEBUG_BREAK()
#else
#define SK_GUARD_FAILED_ACTION() (void)0
#endif

#define SK_INVALID_ENTITY_GUARD2(entity, action)\
if (!(entity))\
{\
	SK_CORE_WARN("InternalCalls: Invalid Entity [{}]", SK_FUNCTION_DECORATED); \
	SK_GUARD_FAILED_ACTION(); \
	action; \
}


#define SK_INVALID_ENTITY_GUARD(entity) SK_INVALID_ENTITY_GUARD2(entity, return false)

#define SK_MISSING_COMPONENT_GUARD(entity, comp)\
if (!(entity).AllOf<comp>())\
{\
	SK_CORE_WARN("InternalCalls: Missing Component [{}]", SK_FUNCTION);\
	SK_GUARD_FAILED_ACTION();\
	return false;\
}

namespace Shark {

	struct EntityBindings
	{
		bool(*HasComponent)(Entity);
		void(*AddComponent)(Entity);
		void(*RemoveComponent)(Entity);
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

		MonoMethod* EntityOnCollishionBegin = nullptr;
		MonoMethod* EntityOnCollishionEnd = nullptr;

	};
	static Scope<MonoGlueData> s_MonoGlue;

	namespace utils {

		static Entity GetEntityActiveScene(uint64_t id)
		{
			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			return scene ? scene->GetEntityByUUID(id) : Entity{};
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

		static const EntityBindings& GetEntityBindings(MonoReflectionType* reflectionType)
		{
			MonoType* t = mono_reflection_type_get_type(reflectionType);
			const char* typeName = mono_type_get_name(t);
			return s_EntityBindings.at(typeName);
		}

		static MonoMethod* GetCoreMethod(const std::string& fullName)
		{
			MonoMethodDesc* methodDesc = mono_method_desc_new(fullName.c_str(), true);
			MonoMethod* method = mono_method_desc_search_in_image(methodDesc, ScriptEngine::GetCoreAssemblyInfo().Image);
			mono_method_desc_free(methodDesc);
			return method;
		}

	}

	void ScriptGlue::Init()
	{
		SK_PROFILE_FUNCTION();

		s_MonoGlue = Scope<MonoGlueData>::Create();

		s_MonoGlue->RaiseOnKeyPressedEvent = utils::GetCoreMethod("Shark.EventHandler:RaiseOnKeyPressed");
		s_MonoGlue->RaiseOnKeyReleasedEvent = utils::GetCoreMethod("Shark.EventHandler:RaiseOnKeyReleased");
		s_MonoGlue->RaiseOnMouseMovedEvent = utils::GetCoreMethod("Shark.EventHandler:RaiseOnMouseMoved");
		s_MonoGlue->RaiseOnMouseButtonPressedEvent = utils::GetCoreMethod("Shark.EventHandler:RaiseOnMouseButtonPressed");
		s_MonoGlue->RaiseOnMouseButtonReleasedEvent = utils::GetCoreMethod("Shark.EventHandler:RaiseOnMouseButtonReleased");
		s_MonoGlue->RaiseOnMouseButtonDoubleClickedEvent = utils::GetCoreMethod("Shark.EventHandler:RaiseOnMouseButtonDoubleClicked");
		s_MonoGlue->RaiseOnMouseScrolledEvent = utils::GetCoreMethod("Shark.EventHandler:RaiseOnMouseScrolled");

		s_MonoGlue->EntityOnCollishionBegin = utils::GetCoreMethod("Shark.Entity:OnCollishionBegin");
		s_MonoGlue->EntityOnCollishionEnd = utils::GetCoreMethod("Shark.Entity:OnCollishionEnd");

		SK_CORE_ASSERT(s_MonoGlue->RaiseOnKeyPressedEvent);
		SK_CORE_ASSERT(s_MonoGlue->RaiseOnKeyReleasedEvent);
		SK_CORE_ASSERT(s_MonoGlue->RaiseOnMouseMovedEvent);
		SK_CORE_ASSERT(s_MonoGlue->RaiseOnMouseButtonPressedEvent);
		SK_CORE_ASSERT(s_MonoGlue->RaiseOnMouseButtonReleasedEvent);
		SK_CORE_ASSERT(s_MonoGlue->RaiseOnMouseButtonDoubleClickedEvent);
		SK_CORE_ASSERT(s_MonoGlue->RaiseOnMouseScrolledEvent);

		SK_CORE_ASSERT(s_MonoGlue->EntityOnCollishionBegin);
		SK_CORE_ASSERT(s_MonoGlue->EntityOnCollishionEnd);

		RegisterComponents();
		RegsiterInternalCalls();
	}

	void ScriptGlue::Shutdown()
	{
		s_MonoGlue = nullptr;
		s_EntityBindings.clear();
	}

	static void CallOnCollishion(Entity entityA, Entity entityB, bool aIsSensor, bool bIsSensor, MonoMethod* collishionMethod)
	{
		SK_PROFILE_FUNCTION();

		if (!s_MonoGlue)
			return;

		const UUID uuidA = entityA.GetUUID();
		const UUID uuidB = entityB.GetUUID();

		GCHandle HandleA = ScriptEngine::ContainsEntityInstance(uuidA) ? ScriptEngine::GetEntityInstance(uuidA) : 0;
		GCHandle HandleB = ScriptEngine::ContainsEntityInstance(uuidB) ? ScriptEngine::GetEntityInstance(uuidB) : 0;

		if (!(HandleA || HandleB))
			return;

		MonoObject* objectA = HandleA ? GCManager::GetManagedObject(HandleA) : nullptr;
		MonoObject* objectB = HandleB ? GCManager::GetManagedObject(HandleB) : nullptr;

		if (objectA && objectB)
		{
			ScriptEngine::InvokeVirtualMethod(objectA, collishionMethod, objectB, bIsSensor);
			ScriptEngine::InvokeVirtualMethod(objectB, collishionMethod, objectA, aIsSensor);
		}
		else
		{
			MonoClass* entityClass = mono_class_from_name_case(ScriptEngine::GetCoreAssemblyInfo().Image, "Shark", "Entity");
			MonoObject* object = mono_object_new(mono_domain_get(), entityClass);
			mono_runtime_object_init(object);
			MonoMethodDesc* desc = mono_method_desc_new(":.ctor(ulong)", false);
			MonoMethod* ctor = mono_method_desc_search_in_class(desc, entityClass);

			UUID uuid;
			MonoObject* entityObject;
			bool isSensor = false;

			if (objectA)
			{
				uuid = uuidB;
				entityObject = objectA;
				isSensor = bIsSensor;
			}
			else
			{
				uuid = uuidA;
				entityObject = objectB;
				isSensor = aIsSensor;
			}

			ScriptEngine::InvokeMethod(object, ctor, uuid);
			ScriptEngine::InvokeVirtualMethod(entityObject, collishionMethod, object, isSensor);
		}

	}

	void ScriptGlue::CallCollishionBegin(Entity entityA, Entity entityB, bool aIsSensor, bool bIsSensor)
	{
		CallOnCollishion(entityA, entityB, aIsSensor, bIsSensor, s_MonoGlue->EntityOnCollishionBegin);
	}

	void ScriptGlue::CallCollishionEnd(Entity entityA, Entity entityB, bool aIsSensor, bool bIsSensor)
	{
		CallOnCollishion(entityA, entityB, aIsSensor, bIsSensor, s_MonoGlue->EntityOnCollishionEnd);
	}

	void ScriptGlue::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		if (!s_MonoGlue)
			return;

		EventDispacher dispacher(event);
		dispacher.DispachEventAlways<KeyPressedEvent>([](auto& e)                { ScriptEngine::InvokeMethod(nullptr, s_MonoGlue->RaiseOnKeyPressedEvent, e.GetKeyCode(), e.IsRepeat()); });
		dispacher.DispachEventAlways<KeyReleasedEvent>([](auto& e)               { ScriptEngine::InvokeMethod(nullptr, s_MonoGlue->RaiseOnKeyReleasedEvent, e.GetKeyCode()); });

		dispacher.DispachEventAlways<MouseMovedEvent>([](auto& e)                { ScriptEngine::InvokeMethod(nullptr, s_MonoGlue->RaiseOnMouseMovedEvent, e.GetMousePos()); });
		dispacher.DispachEventAlways<MouseButtonPressedEvent>([](auto& e)        { ScriptEngine::InvokeMethod(nullptr, s_MonoGlue->RaiseOnMouseButtonPressedEvent, e.GetButton(), e.GetMousePos()); });
		dispacher.DispachEventAlways<MouseButtonReleasedEvent>([](auto& e)       { ScriptEngine::InvokeMethod(nullptr, s_MonoGlue->RaiseOnMouseButtonReleasedEvent, e.GetButton(), e.GetMousePos()); });
		dispacher.DispachEventAlways<MouseButtonDoubleClickedEvent>([](auto& e)  { ScriptEngine::InvokeMethod(nullptr, s_MonoGlue->RaiseOnMouseButtonDoubleClickedEvent, e.GetButton(), e.GetMousePos()); });
		dispacher.DispachEventAlways<MouseScrolledEvent>([](auto& e)             { ScriptEngine::InvokeMethod(nullptr, s_MonoGlue->RaiseOnMouseScrolledEvent, e.GetDelta(), e.GetMousePos()); });
	}

	void ScriptGlue::RegisterComponents()
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

	void ScriptGlue::RegsiterInternalCalls()
	{
		SK_PROFILE_FUNCTION();

		SK_ADD_INTERNAL_CALL(Application_GetWidth);
		SK_ADD_INTERNAL_CALL(Application_GetHeight);

		SK_ADD_INTERNAL_CALL(Log_LogLevel);

		SK_ADD_INTERNAL_CALL(Input_KeyPressed);
		SK_ADD_INTERNAL_CALL(Input_MouseButtonPressed);
		SK_ADD_INTERNAL_CALL(Input_GetMousePos);

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

		SK_ADD_INTERNAL_CALL(Entity_HasComponent);
		SK_ADD_INTERNAL_CALL(Entity_AddComponent);

		SK_ADD_INTERNAL_CALL(TagComponent_GetTag);
		SK_ADD_INTERNAL_CALL(TagComponent_SetTag);

		SK_ADD_INTERNAL_CALL(TransformComponent_GetTranslation);
		SK_ADD_INTERNAL_CALL(TransformComponent_SetTranslation);
		SK_ADD_INTERNAL_CALL(TransformComponent_GetRotation);
		SK_ADD_INTERNAL_CALL(TransformComponent_SetRotation);
		SK_ADD_INTERNAL_CALL(TransformComponent_GetScale);
		SK_ADD_INTERNAL_CALL(TransformComponent_SetScale);
		SK_ADD_INTERNAL_CALL(TransformComponent_GetLocalTransform);
		SK_ADD_INTERNAL_CALL(TransformComponent_SetLocalTransform);
		SK_ADD_INTERNAL_CALL(TransformComponent_GetWorldTransform);
		SK_ADD_INTERNAL_CALL(TransformComponent_SetWorldTransform);

		SK_ADD_INTERNAL_CALL(SpriteRendererComponent_GetColor);
		SK_ADD_INTERNAL_CALL(SpriteRendererComponent_SetColor);
		SK_ADD_INTERNAL_CALL(SpriteRendererComponent_SetTextureHandle);
		SK_ADD_INTERNAL_CALL(SpriteRendererComponent_GetTextureHandle);
		SK_ADD_INTERNAL_CALL(SpriteRendererComponent_GetTilingFactor);
		SK_ADD_INTERNAL_CALL(SpriteRendererComponent_SetTilingFactor);

		SK_ADD_INTERNAL_CALL(CircleRendererComponent_GetColor);
		SK_ADD_INTERNAL_CALL(CircleRendererComponent_SetColor);
		SK_ADD_INTERNAL_CALL(CircleRendererComponent_GetThickness);
		SK_ADD_INTERNAL_CALL(CircleRendererComponent_SetThickness);
		SK_ADD_INTERNAL_CALL(CircleRendererComponent_GetFade);
		SK_ADD_INTERNAL_CALL(CircleRendererComponent_SetFade);

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

		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetSensor);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_IsSensor);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetDensity);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetDensity);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetFriction);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetFriction);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetRestitution);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetRestitution);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetRestitutionThreshold);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetRestitutionThreshold);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetSize);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetSize);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetOffset);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetOffset);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_GetRotation);
		SK_ADD_INTERNAL_CALL(BoxCollider2DComponent_SetRotation);

		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetSensor);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_IsSensor);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetDensity);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetDensity);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetFriction);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetFriction);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetRestitution);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetRestitution);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetRestitutionThreshold);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetRestitutionThreshold);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetRadius);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetRadius);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetOffset);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetOffset);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_GetRotation);
		SK_ADD_INTERNAL_CALL(CircleCollider2DComponent_SetRotation);

		SK_ADD_INTERNAL_CALL(ResourceManager_GetAssetHandleFromFilePath);

		SK_ADD_INTERNAL_CALL(EditorUI_BeginWindow);
		SK_ADD_INTERNAL_CALL(EditorUI_EndWindow);
		SK_ADD_INTERNAL_CALL(EditorUI_Text);
		SK_ADD_INTERNAL_CALL(EditorUI_NewLine);
		SK_ADD_INTERNAL_CALL(EditorUI_Separator);

	}

	namespace InternalCalls {

		#pragma region Application

		uint32_t Application_GetWidth()
		{
			if (Application::Get().GetSpecs().EnableImGui)
			{
				ImGuiWindow* mainViewport = ImGui::FindWindowByName("MainViewport");
				SK_CORE_ASSERT(mainViewport);
				return (uint32_t)mainViewport->ContentRegionRect.GetWidth();
			}

			return Application::Get().GetWindow().GetWidth();
		}

		uint32_t Application_GetHeight()
		{
			if (Application::Get().GetSpecs().EnableImGui)
			{
				ImGuiWindow* mainViewport = ImGui::FindWindowByName("MainViewport");
				SK_CORE_ASSERT(mainViewport);
				return (uint32_t)mainViewport->ContentRegionRect.GetHeight();
			}

			return Application::Get().GetWindow().GetHeight();
		}

		#pragma endregion

		#pragma region Log

		void Log_LogLevel(Log::Level level, MonoString* message)
		{
			char* msg = mono_string_to_utf8(message);
			SK_CONSOLE_LOG(level, msg);
		}

		#pragma endregion

		#pragma region Input

		bool Input_KeyPressed(KeyCode key)
		{
			auto& app = Application::Get();

			if (!app.GetWindow().IsFocused())
				return false;

			if (app.GetSpecs().EnableImGui && ImGui::GetIO().WantCaptureKeyboard)
			{
				const ImGuiWindow* activeWindow = GImGui->ActiveIdWindow;
				if (activeWindow && strcmp(activeWindow->Name, "MainViewport") != 0)
					return false;
			}

			return Input::KeyPressed(key);
		}

		bool Input_MouseButtonPressed(MouseButton::Type button)
		{
			auto& app = Application::Get();

			if (!app.GetWindow().IsFocused())
				return false;

			if (Application::Get().GetSpecs().EnableImGui && ImGui::GetIO().WantCaptureMouse)
			{
				const ImGuiWindow* hoveredWindow = GImGui->HoveredWindow;
				if (hoveredWindow && strcmp(hoveredWindow->Name, "MainViewport") != 0)
					return false;
			}

			return Input::MousePressed(button);
		}

		bool Input_GetMousePos(glm::ivec2* out_MousePos)
		{
			auto p = Input::GlobalMousePos();

			if (Application::Get().GetSpecs().EnableImGui)
			{
				ImGuiWindow* viewportWindow = ImGui::FindWindowByName("MainViewport");
				if (viewportWindow)
				{
					p.x -= (int)viewportWindow->Pos.x;
					p.y -= (int)viewportWindow->Pos.y;
				}
			}

			*out_MousePos = { p.x, p.y };
			return true;
		}

		#pragma endregion

		#pragma region Matrix4

		void Matrix4_Inverse(glm::mat4* matrix, glm::mat4* out_Result)
		{
			*out_Result = glm::inverse(*matrix);
		}

		glm::mat4 Matrix4_Matrix4MulMatrix4(glm::mat4* lhs, glm::mat4* rhs)
		{
			return *lhs * *rhs;
		}

		glm::vec4 Matrix4_Matrix4MulVector4(glm::mat4* lhs, glm::vec4* rhs)
		{
			return *lhs * *rhs;
		}

		#pragma endregion

		#pragma region Scene

		MonoObject* Scene_InstantiateScript(MonoReflectionType* scriptType, MonoString* name)
		{
			MonoType* monoType = mono_reflection_type_get_type(scriptType);
			const char* scriptTypeName = mono_type_get_name(monoType);

			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			Entity newEntity = scene->CreateEntity(mono_string_to_utf8(name));
			auto& comp = newEntity.AddComponent<ScriptComponent>();
			comp.ScriptName = scriptTypeName;
			comp.IsExisitingScript = true;

			if (ScriptEngine::InstantiateEntity(newEntity, true))
			{
				comp.IsExisitingScript = true;
				GCHandle gcHandle = ScriptEngine::GetEntityInstance(newEntity.GetUUID());
				return GCManager::GetManagedObject(gcHandle);
			}
			return nullptr;
		} 

		void Scene_CreateEntity(MonoString* name, uint64_t entityID, uint64_t* out_EntityID)
		{
			Ref<Scene> scene = ScriptEngine::GetActiveScene();

			const char* entityName = mono_string_to_utf8(name);
			Entity entity = scene->CreateEntityWithUUID(entityID, entityName ? entityName : std::string{});
			*out_EntityID = (uint64_t)entity.GetUUID();
		}

		void Scene_DestroyEntity(uint64_t entityID)
		{
			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			auto& queue = scene->GetPostUpdateQueue();
			queue.push([scene, entityID]()
			{
				Entity entity = scene->GetEntityByUUID(entityID);
				SK_INVALID_ENTITY_GUARD2(entity, return);
				scene->DestroyEntity(entity);
			});

		}

		void Scene_CloneEntity(uint64_t entityID, UUID* out_UUID)
		{
			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			Entity entity = scene->GetEntityByUUID(entityID);
			SK_INVALID_ENTITY_GUARD2(entity, return);
			Entity clonedEntity = scene->CloneEntity(entity);
			*out_UUID = clonedEntity.GetUUID();
		}

		MonoObject* Scene_GetScriptObject(uint64_t scriptEntityID)
		{
			if (!ScriptEngine::ContainsEntityInstance(scriptEntityID))
				return nullptr;

			GCHandle gcHandle = ScriptEngine::GetEntityInstance(scriptEntityID);
			return GCManager::GetManagedObject(gcHandle);
		}

		bool Scene_IsValidEntityHandle(uint64_t entityID)
		{
			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			return scene->GetEntityByUUID(entityID);
		}

		bool Scene_GetActiveCameraUUID(uint64_t* out_CameraID)
		{
			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			*out_CameraID = (uint64_t)scene->GetActiveCameraUUID();
			return true;
		}

		bool Scene_GetUUIDFromTag(MonoString* tag, uint64_t* out_EntityID)
		{
			const char* cStr = mono_string_to_utf8(tag);

			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			auto view = scene->GetAllEntitysWith<TagComponent>();
			for (auto entityID : view)
			{
				Entity entity{ entityID, scene };
				if (entity.GetName() == cStr)
				{
					*out_EntityID = (uint64_t)entity.GetUUID();
					return true;
				}
			}
			return false;
		}

		#pragma endregion

		#pragma region Entity

		bool Entity_HasComponent(uint64_t id, MonoReflectionType* type)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			
			auto& bindings = utils::GetEntityBindings(type);
			return bindings.HasComponent(entity);
		}

		void Entity_AddComponent(uint64_t id, MonoReflectionType* type)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD2(entity, return);

			auto& bindings = utils::GetEntityBindings(type);
			if (bindings.HasComponent(entity))
				return;

			bindings.AddComponent(entity);
		}

		void Entity_RemoveComponent(uint64_t id, MonoReflectionType* type)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD2(entity, return);

			auto& bindings = utils::GetEntityBindings(type);
			if (!bindings.HasComponent(entity))
				return;

			bindings.RemoveComponent(entity);
		}

		#pragma endregion

		#pragma region TagComponent

		bool TagComponent_GetTag(uint64_t id, MonoString** out_Tag)
		{
			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			Entity entity = scene->GetEntityByUUID(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_CORE_ASSERT(entity.AllOf<TagComponent>(), "All entities need to have a TagComponent");
			*out_Tag = mono_string_new(mono_domain_get(), entity.GetName().c_str());
			return true;
		}

		bool TagComponent_SetTag(uint64_t id, MonoString* tag)
		{
			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			Entity entity = scene->GetEntityByUUID(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_CORE_ASSERT(entity.AllOf<TagComponent>(), "All entities need to have a TagComponent");
			char* str = mono_string_to_utf8(tag);
			if (str)
			{
				entity.GetComponent<TagComponent>().Tag = str;
				return true;
			}
			return false;
		}

		#pragma endregion

		#pragma region TransformComponent

		bool TransformComponent_GetTranslation(uint64_t id, glm::vec3* out_Translation)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			auto& transform = entity.Transform();
			*out_Translation = transform.Translation;
			return true;
		}

		bool TransformComponent_SetTranslation(uint64_t id, glm::vec3* translation)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			auto& transform = entity.Transform();
			transform.Translation = *translation;
			return true;
		}

		bool TransformComponent_GetRotation(uint64_t id, glm::vec3* out_Rotation)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			auto& transform = entity.Transform();
			*out_Rotation = transform.Rotation;
			return true;
		}

		bool TransformComponent_SetRotation(uint64_t id, glm::vec3* rotation)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			auto& transform = entity.Transform();
			transform.Rotation = *rotation;
			return true;
		}

		bool TransformComponent_GetScale(uint64_t id, glm::vec3* out_Scaling)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			auto& transform = entity.Transform();
			*out_Scaling = transform.Scale;
			return true;
		}

		bool TransformComponent_SetScale(uint64_t id, glm::vec3* scaling)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			auto& transform = entity.Transform();
			transform.Scale = *scaling;
			return true;
		}

		bool TransformComponent_GetLocalTransform(uint64_t id, TransformComponent* out_LocalTransform)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);

			*out_LocalTransform = entity.Transform();
			return true;

		}

		bool TransformComponent_SetLocalTransform(uint64_t id, TransformComponent* localTransform)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);

			entity.Transform() = *localTransform;
			return true;
		}

		bool TransformComponent_GetWorldTransform(uint64_t id, TransformComponent* out_WorldTransform)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);

			if (!entity.HasParent())
			{
				*out_WorldTransform = entity.Transform();
				return true;
			}

			Entity parent = entity.ParentEntity();
			glm::mat4 localToWorld = parent.CalcWorldTransform();
			
			auto& tf = entity.Transform();
			*out_WorldTransform = entity.WorldTransform();
			return true;
		}

		bool TransformComponent_SetWorldTransform(uint64_t id, TransformComponent* worldTransform)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);

			if (!entity.HasParent())
			{
				entity.Transform() = *worldTransform;
				return true;
			}


			Entity parent = entity.ParentEntity();
			TransformComponent parentWorldTF = parent.WorldTransform();
			entity.Transform() = TransformUtils::ToLocal(*worldTransform, parentWorldTF);

			const auto a = entity.Transform().Translation;
			const auto m = glm::fmod(a, 1.0f);
			const auto abs = glm::abs(m);
			const auto eequal = glm::equal(abs, glm::vec3(glm::epsilon<float>()));
			if (glm::any(eequal))
			{
				SK_CORE_INFO("InternalCalls SetWorldTransform, Tralstion eqsilonEqual");
			}

			return true;
		}

		#pragma endregion

		#pragma region SpriteRendererComponent

		bool SpriteRendererComponent_GetColor(uint64_t id, glm::vec4* out_Color)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, SpriteRendererComponent);
			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			*out_Color = spriteRenderer.Color;
			return true;
		}

		bool SpriteRendererComponent_SetColor(uint64_t id, glm::vec4* color)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, SpriteRendererComponent);
			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			spriteRenderer.Color = *color;
			return true;
		}

		bool SpriteRendererComponent_GetTextureHandle(uint64_t id, AssetHandle* out_TextureHandle)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, SpriteRendererComponent);
			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			*out_TextureHandle = spriteRenderer.TextureHandle;
			return true;
		}

		bool SpriteRendererComponent_SetTextureHandle(uint64_t id, AssetHandle* textureHandle)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, SpriteRendererComponent);
			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			spriteRenderer.TextureHandle = *textureHandle;
			return true;
		}

		bool SpriteRendererComponent_GetTilingFactor(uint64_t id, float* out_TilingFactor)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, SpriteRendererComponent);
			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			*out_TilingFactor = spriteRenderer.TilingFactor;
			return true;
		}

		bool SpriteRendererComponent_SetTilingFactor(uint64_t id, float tilingFactor)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, SpriteRendererComponent);
			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			spriteRenderer.TilingFactor = tilingFactor;
			return true;
		}

		#pragma endregion

		#pragma region CricleRendererComponent

		bool CircleRendererComponent_GetColor(uint64_t id, glm::vec4* out_Color)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleRendererComponent);
			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			*out_Color = circleRenderer.Color;
			return true;
		}

		bool CircleRendererComponent_SetColor(uint64_t id, glm::vec4* color)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleRendererComponent);
			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			circleRenderer.Color = *color;
			return true;
		}

		bool CircleRendererComponent_GetThickness(uint64_t id, float* out_Thickness)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleRendererComponent);
			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			*out_Thickness = circleRenderer.Thickness;
			return true;
		}

		bool CircleRendererComponent_SetThickness(uint64_t id, float thickness)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleRendererComponent);
			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			circleRenderer.Thickness = thickness;
			return true;
		}

		bool CircleRendererComponent_GetFade(uint64_t id, float* out_Fade)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleRendererComponent);
			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			*out_Fade = circleRenderer.Fade;
			return true;
		}

		bool CircleRendererComponent_SetFade(uint64_t id, float fade)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleRendererComponent);
			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			circleRenderer.Fade = fade;
			return true;
		}

		#pragma endregion

		#pragma region CameraComponent

		bool CameraComponent_GetProjection(uint64_t id, glm::mat4* out_Projection)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			*out_Projection = camera.GetProjection();
			return true;
		}

		bool CameraComponent_SetProjection(uint64_t id, glm::mat4* projection)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetProjection(*projection);
			return true;
		}

		bool CameraComponent_GetProjectionType(uint64_t id, SceneCamera::Projection* out_ProjectionType)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			*out_ProjectionType = camera.GetProjectionType();
			return true;
		}

		bool CameraComponent_SetProjectionType(uint64_t id, SceneCamera::Projection projectionType)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetProjectionType(projectionType);
			return true;
		}
		
		bool CameraComponent_SetPerspective(uint64_t id, float aspectratio, float fov, float clipnear, float clipfar)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspective(aspectratio, fov, clipnear, clipfar);
			return true;
		}

		bool CameraComponent_SetOrthographic(uint64_t id, float aspectratio, float zoom, float clipnear, float clipfar)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographic(aspectratio, zoom, clipnear, clipfar);
			return true;
		}

		bool CameraComponent_GetAspectratio(uint64_t id, float* out_Aspectratio)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			*out_Aspectratio = camera.GetAspectratio();
			return true;
		}

		bool CameraComponent_SetAspectratio(uint64_t id, float aspectratio)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetAspectratio(aspectratio);
			return true;
		}

		bool CameraComponent_GetPerspectiveFOV(uint64_t id, float* out_FOV)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			*out_FOV = camera.GetPerspectiveFOV();
			return true;
		}

		bool CameraComponent_SetPerspectiveFOV(uint64_t id, float fov)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspectiveFOV(fov);
			return true;
		}

		bool CameraComponent_GetPerspectiveNear(uint64_t id, float* out_Near)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			*out_Near = camera.GetPerspectiveNear();
			return true;
		}

		bool CameraComponent_SetPerspectiveNear(uint64_t id, float clipnear)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspectiveNear(clipnear);
			return true;
		}

		bool CameraComponent_GetPerspectiveFar(uint64_t id, float* out_Far)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			*out_Far = camera.GetPerspectiveFar();
			return true;
		}

		bool CameraComponent_SetPerspectiveFar(uint64_t id, float clipfar)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspectiveFar(clipfar);
			return true;
		}

		bool CameraComponent_GetOrthographicZoom(uint64_t id, float* out_Zoom)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			*out_Zoom = camera.GetOrthographicZoom();
			return true;
		}

		bool CameraComponent_SetOrthographicZoom(uint64_t id, float zoom)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographicZoom(zoom);
			return true;
		}

		bool CameraComponent_GetOrthographicNear(uint64_t id, float* out_Near)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			*out_Near = camera.GetOrthographicNear();
			return true;
		}

		bool CameraComponent_SetOrthographicNear(uint64_t id, float clipnear)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographicNear(clipnear);
			return true;
		}

		bool CameraComponent_GetOrthographicFar(uint64_t id, float* out_Far)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			*out_Far = camera.GetOrthographicFar();
			return true;
		}
		
		bool CameraComponent_SetOrthographicFar(uint64_t id, float clipfar)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CameraComponent);
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographicFar(clipfar);
			return true;
		}

		#pragma endregion

		#pragma region RigidBody2DComponent

		bool RigidBody2DComponent_GetBodyType(uint64_t id, RigidBody2DComponent::BodyType* out_BodyType)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			*out_BodyType = utils::b2BodyTypeToSharkBodyType(body->GetType());
			return true;
		}

		bool RigidBody2DComponent_SetBodyType(uint64_t id, RigidBody2DComponent::BodyType bodyType)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			body->SetType(utils::SharkBodyTypeTob2BodyType(bodyType));
			return true;
		}

		bool RigidBody2DComponent_GetTransform(uint64_t id, RigidBody2DTransform* out_Transform)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			const auto& tf = body->GetTransform();
			
			RigidBody2DTransform transform;
			transform.Position = { tf.p.x, tf.p.y };
			transform.Angle = tf.q.GetAngle();
			*out_Transform = transform;
			return true;
		}

		bool RigidBody2DComponent_SetTransform(uint64_t id, RigidBody2DTransform* transform)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			const b2Vec2 pos = { transform->Position.x, transform->Position.y };
			body->SetTransform(pos, transform->Angle);
			return true;
		}

		bool RigidBody2DComponent_SetPosition(uint64_t id, glm::vec2* position)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			body->SetTransform({ position->x, position->y }, body->GetAngle());
			return true;
		}

		bool RigidBody2DComponent_SetRotation(uint64_t id, float rotation)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			body->SetTransform(body->GetPosition(), rotation);
			return true;
		}

		bool RigidBody2DComponent_GetLocalCenter(uint64_t id, glm::vec2* out_LocalCenter)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			const b2Vec2 lc = body->GetLocalCenter();
			*out_LocalCenter = { lc.x, lc.y };
			return true;
		}

		bool RigidBody2DComponent_GetWorldCenter(uint64_t id, glm::vec2* out_WorldCenter)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			const b2Vec2 wc = body->GetWorldCenter();
			*out_WorldCenter = { wc.x, wc.y };
			return true;
		}

		bool RigidBody2DComponent_GetLinearVelocity(uint64_t id, glm::vec2* out_LinearVelocity)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			const b2Vec2& lv = body->GetLinearVelocity();
			*out_LinearVelocity = { lv.x, lv.y };
			return true;
		}

		bool RigidBody2DComponent_SetLinearVelocity(uint64_t id, glm::vec2* linearVelocity)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			body->SetLinearVelocity({ linearVelocity->x, linearVelocity->y });
			return true;
		}

		bool RigidBody2DComponent_GetAngularVelocity(uint64_t id, float* out_AngularVelocity)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			*out_AngularVelocity = body->GetAngularVelocity();
			return true;
		}

		bool RigidBody2DComponent_SetAngularVelocity(uint64_t id, float angularVelocity)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			body->SetAngularVelocity(angularVelocity);
			return true;
		}

		bool RigidBody2DComponent_ApplyForce(uint64_t id, glm::vec2* force, glm::vec2* point, PhysicsForce2DType forceType)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			if (forceType == PhysicsForce2DType::Force)
				body->ApplyForce({ force->x, force->y }, { point->x, point->y }, true);
			else
				body->ApplyLinearImpulse({ force->x, force->y }, { point->x, point->y }, true);

			return true;
		}

		bool RigidBody2DComponent_ApplyForceToCenter(uint64_t id, glm::vec2* force, PhysicsForce2DType forceType)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			if (forceType == PhysicsForce2DType::Force)
				body->ApplyForceToCenter({ force->x, force->y }, true);
			else
				body->ApplyLinearImpulseToCenter({ force->x, force->y }, true);

			return true;
		}

		bool RigidBody2DComponent_ApplyTorque(uint64_t id, float torque, PhysicsForce2DType forceType)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			if (forceType == PhysicsForce2DType::Force)
				body->ApplyTorque(torque, true);
			else
				body->ApplyAngularImpulse(torque, true);

			return true;
		}

		bool RigidBody2DComponent_GetGravityScale(uint64_t id, float* out_GravityScale)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			*out_GravityScale = body->GetGravityScale();
			return true;
		}

		bool RigidBody2DComponent_SetGravityScale(uint64_t id, float gravityScale)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);

			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			comp.GravityScale = gravityScale;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			body->SetGravityScale(gravityScale);
			return true;
		}

		bool RigidBody2DComponent_GetLinearDamping(uint64_t id, float* out_LinearDamping)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			*out_LinearDamping = body->GetLinearDamping();
			return true;
		}

		bool RigidBody2DComponent_SetLinearDamping(uint64_t id, float linearDamping)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			body->SetLinearDamping(linearDamping);
			return true;
		}

		bool RigidBody2DComponent_GetAngularDamping(uint64_t id, float* out_AngularDamping)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			*out_AngularDamping = body->GetAngularDamping();
			return true;
		}

		bool RigidBody2DComponent_SetAngularDamping(uint64_t id, float angularDamping)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			body->SetAngularDamping(angularDamping);
			return true;
		}

		bool RigidBody2DComponent_IsBullet(uint64_t id, bool* out_IsBullet)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			*out_IsBullet = body->IsBullet();
			return true;
		}

		bool RigidBody2DComponent_SetBullet(uint64_t id, bool bullet)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);

			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			comp.IsBullet = bullet;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			body->SetBullet(bullet);
			return true;
		}

		bool RigidBody2DComponent_IsSleepingAllowed(uint64_t id, bool* out_SleepingAllowed)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			*out_SleepingAllowed = body->IsSleepingAllowed();
			return true;
		}

		bool RigidBody2DComponent_SetSleepingAllowed(uint64_t id, bool sleepingAllowed)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);

			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			comp.AllowSleep = sleepingAllowed;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			body->SetSleepingAllowed(sleepingAllowed);
			return true;
		}

		bool RigidBody2DComponent_IsAwake(uint64_t id, bool* out_IsAwake)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			*out_IsAwake = body->IsAwake();
			return true;
		}

		bool RigidBody2DComponent_SetAwake(uint64_t id, bool awake)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);

			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			comp.Awake = awake;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			body->SetAwake(awake);
			return true;
		}

		bool RigidBody2DComponent_IsEnabled(uint64_t id, bool* out_IsEnabled)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			*out_IsEnabled = body->IsEnabled();
			return true;
		}

		bool RigidBody2DComponent_SetEnabled(uint64_t id, bool enabled)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);

			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			comp.Enabled = enabled;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			body->SetEnabled(enabled);
			return true;
		}

		bool RigidBody2DComponent_IsFixedRotation(uint64_t id, bool* out_FixedRotation)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);
			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			*out_FixedRotation = body->IsFixedRotation();
			return true;
		}

		bool RigidBody2DComponent_SetFixedRotation(uint64_t id, bool fixedRotation)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, RigidBody2DComponent);

			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			comp.FixedRotation = fixedRotation;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			body->SetFixedRotation(fixedRotation);
			return true;
		}

		#pragma endregion

		#pragma region BoxCollider2DComponent

		bool BoxCollider2DComponent_SetSensor(uint64_t id, bool sensor)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			comp.IsSensor = sensor;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			fixture->SetSensor(sensor);
			return true;
		}

		bool BoxCollider2DComponent_IsSensor(uint64_t id, bool* out_IsSensor)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			*out_IsSensor = fixture->IsSensor();
			return true;
		}

		bool BoxCollider2DComponent_SetDensity(uint64_t id, float density)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			comp.Density = density;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			fixture->SetDensity(density);
			return true;
		}

		bool BoxCollider2DComponent_GetDensity(uint64_t id, bool* out_Density)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			*out_Density = fixture->GetDensity();
			return true;
		}

		bool BoxCollider2DComponent_SetFriction(uint64_t id, float friction)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			comp.Friction = friction;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			fixture->SetFriction(friction);
			return true;
		}

		bool BoxCollider2DComponent_GetFriction(uint64_t id, float* out_Friction)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			*out_Friction = fixture->GetFriction();
			return true;
		}

		bool BoxCollider2DComponent_SetRestitution(uint64_t id, float restitution)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			comp.Restitution = restitution;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			fixture->SetRestitution(restitution);
			return true;
		}

		bool BoxCollider2DComponent_GetRestitution(uint64_t id, float* out_Restitution)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			*out_Restitution = fixture->GetRestitution();
			return true;
		}

		bool BoxCollider2DComponent_SetRestitutionThreshold(uint64_t id, float restitutionThreshold)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			comp.RestitutionThreshold = restitutionThreshold;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			fixture->SetRestitutionThreshold(restitutionThreshold);
			return true;
		}

		bool BoxCollider2DComponent_GetRestitutionThreshold(uint64_t id, float* out_RestitutionThreshold)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			*out_RestitutionThreshold = fixture->GetRestitutionThreshold();
			return true;
		}

		bool BoxCollider2DComponent_GetSize(uint64_t id, glm::vec2* out_Size)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			*out_Size = comp.Size;
			return true;
		}

		bool BoxCollider2DComponent_SetSize(uint64_t id, glm::vec2* size)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			const auto& transform = entity.Transform();
			comp.Size = *size;

			if (!comp.RuntimeCollider)
				return true;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_polygon);
			b2PolygonShape* shape = (b2PolygonShape*)comp.RuntimeCollider->GetShape();
			shape->SetAsBox(comp.Size.x * transform.Scale.x, comp.Size.y * transform.Scale.y, { comp.Offset.x, comp.Offset.y }, comp.Rotation);
			return true;
		}

		bool BoxCollider2DComponent_GetOffset(uint64_t id, glm::vec2* out_Offset)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			*out_Offset = comp.Offset;
			return true;
		}

		bool BoxCollider2DComponent_SetOffset(uint64_t id, glm::vec2* offset)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			const auto& transform = entity.Transform();
			comp.Offset = *offset;

			if (!comp.RuntimeCollider)
				return true;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_polygon);
			b2PolygonShape* shape = (b2PolygonShape*)comp.RuntimeCollider->GetShape();
			shape->SetAsBox(comp.Size.x * transform.Scale.x, comp.Size.y * transform.Scale.y, { comp.Offset.x, comp.Offset.y }, comp.Rotation);
			return true;
		}

		bool BoxCollider2DComponent_GetRotation(uint64_t id, float* out_Rotation)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			*out_Rotation = comp.Rotation;
			return true;
		}

		bool BoxCollider2DComponent_SetRotation(uint64_t id, float rotation)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, BoxCollider2DComponent);

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			const auto& transform = entity.Transform();
			comp.Rotation = rotation;

			if (!comp.RuntimeCollider)
				return true;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_polygon);
			b2PolygonShape* shape = (b2PolygonShape*)comp.RuntimeCollider->GetShape();
			shape->SetAsBox(comp.Size.x * transform.Scale.x, comp.Size.y * transform.Scale.y, { comp.Offset.x, comp.Offset.y }, comp.Rotation);
			return true;
		}

		#pragma endregion

		#pragma region CircleCollider2DComponent

		bool CircleCollider2DComponent_SetSensor(uint64_t id, bool sensor)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			comp.IsSensor = sensor;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			fixture->SetSensor(sensor);
			return true;
		}

		bool CircleCollider2DComponent_IsSensor(uint64_t id, bool* out_IsSensor)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			*out_IsSensor = fixture->IsSensor();
			return true;
		}

		bool CircleCollider2DComponent_SetDensity(uint64_t id, float density)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			comp.Density = density;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			fixture->SetDensity(density);
			return true;
		}

		bool CircleCollider2DComponent_GetDensity(uint64_t id, bool* out_Density)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			*out_Density = fixture->GetDensity();
			return true;
		}

		bool CircleCollider2DComponent_SetFriction(uint64_t id, float friction)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			comp.Friction = friction;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			fixture->SetFriction(friction);
			return true;
		}

		bool CircleCollider2DComponent_GetFriction(uint64_t id, float* out_Friction)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			*out_Friction = fixture->GetFriction();
			return true;
		}

		bool CircleCollider2DComponent_SetRestitution(uint64_t id, float restitution)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			comp.Restitution = restitution;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			fixture->SetRestitution(restitution);
			return true;
		}

		bool CircleCollider2DComponent_GetRestitution(uint64_t id, float* out_Restitution)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			*out_Restitution = fixture->GetRestitution();
			return true;
		}

		bool CircleCollider2DComponent_SetRestitutionThreshold(uint64_t id, float restitutionThreshold)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			comp.RestitutionThreshold = restitutionThreshold;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			fixture->SetRestitutionThreshold(restitutionThreshold);
			return true;
		}

		bool CircleCollider2DComponent_GetRestitutionThreshold(uint64_t id, float* out_RestitutionThreshold)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			*out_RestitutionThreshold = fixture->GetRestitutionThreshold();
			return true;
		}

		bool CircleCollider2DComponent_GetRadius(uint64_t id, float* out_Radius)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			*out_Radius = comp.Radius;
			return true;
		}

		bool CircleCollider2DComponent_SetRadius(uint64_t id, float Radius)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			const auto& transform = entity.Transform();
			comp.Radius = Radius;

			if (!comp.RuntimeCollider)
				return true;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_circle);
			b2CircleShape* shape = (b2CircleShape*)comp.RuntimeCollider->GetShape();
			shape->m_radius = comp.Radius * transform.Scale.x;
			shape->m_p = { comp.Offset.x, comp.Offset.y };
			return true;
		}

		bool CircleCollider2DComponent_GetOffset(uint64_t id, glm::vec2* out_Offsets)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			*out_Offsets = comp.Offset;
			return true;
		}

		bool CircleCollider2DComponent_SetOffset(uint64_t id, glm::vec2* offset)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			const auto& transform = entity.Transform();
			comp.Offset = *offset;

			if (!comp.RuntimeCollider)
				return true;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_circle);
			b2CircleShape* shape = (b2CircleShape*)comp.RuntimeCollider->GetShape();
			shape->m_radius = comp.Radius * transform.Scale.x;
			shape->m_p = { comp.Offset.x, comp.Offset.y };
			return true;
		}

		bool CircleCollider2DComponent_GetRotation(uint64_t id, float* out_Rotation)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			*out_Rotation = comp.Rotation;
			return true;
		}

		bool CircleCollider2DComponent_SetRotation(uint64_t id, float rotation)
		{
			Entity entity = utils::GetEntityActiveScene(id);
			SK_INVALID_ENTITY_GUARD(entity);
			SK_MISSING_COMPONENT_GUARD(entity, CircleCollider2DComponent);

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			const auto& transform = entity.Transform();
			comp.Rotation = rotation;

			if (!comp.RuntimeCollider)
				return true;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_circle);
			b2CircleShape* shape = (b2CircleShape*)comp.RuntimeCollider->GetShape();
			shape->m_radius = comp.Radius * transform.Scale.x;
			shape->m_p = { comp.Offset.x, comp.Offset.y };
			return true;
		}

		#pragma endregion

		#pragma region ResoureManager

		bool ResourceManager_GetAssetHandleFromFilePath(MonoString* filePath, AssetHandle* out_AssetHandle)
		{
			const wchar_t* cFilePath = mono_string_to_utf16(filePath);
			if (!cFilePath)
				return false;

			*out_AssetHandle = ResourceManager::GetAssetHandleFromFilePath(cFilePath);
			return true;
		}

		#pragma endregion

		#pragma region EditorUI

		bool EditorUI_BeginWindow(MonoString* windowTitle)
		{
			auto& app = Application::Get();
			if (!(app.GetSpecs().EnableImGui && app.GetImGuiLayer().InFrame()))
				return false;

			char* cWindowTitle = mono_string_to_utf8(windowTitle);
			return ImGui::Begin(cWindowTitle, nullptr, ImGuiWindowFlags_NoSavedSettings);
		}

		void EditorUI_EndWindow()
		{
			auto& app = Application::Get();
			if (!(app.GetSpecs().EnableImGui && app.GetImGuiLayer().InFrame()))
				return;

			ImGui::End();
		}

		void EditorUI_Text(MonoString* text)
		{
			auto& app = Application::Get();
			if (!(app.GetSpecs().EnableImGui && app.GetImGuiLayer().InFrame()))
				return;

			char* cText = mono_string_to_utf8(text);
			ImGui::Text(cText);
		}

		void EditorUI_NewLine()
		{
			auto& app = Application::Get();
			if (!(app.GetSpecs().EnableImGui && app.GetImGuiLayer().InFrame()))
				return;

			ImGui::NewLine();
		}

		void EditorUI_Separator()
		{
			auto& app = Application::Get();
			if (!(app.GetSpecs().EnableImGui && app.GetImGuiLayer().InFrame()))
				return;

			ImGui::Separator();
		}

		#pragma endregion

	}

}
