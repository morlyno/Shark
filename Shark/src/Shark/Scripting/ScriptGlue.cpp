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

#include "Shark/UI/UI.h"

#include "Shark/Math/Math.h"
#include "Shark/Utils/MemoryUtils.h"
#include "Shark/Debug/Instrumentor.h"

#include <mono/metadata/appdomain.h>
#include <mono/metadata/object.h>
#include "mono/metadata/class.h"
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

namespace Shark {

	struct EntityBindings
	{
		bool (*HasComponent)(Entity);
		void (*AddComponent)(Entity);
		void (*RemoveComponent)(Entity);
	};

	static std::unordered_map<MonoType*, EntityBindings> s_EntityBindings;

	struct MonoGlueData
	{
		UnmanagedThunk<MonoObject*, MonoObject*> EntityOnCollishionBegin;
		UnmanagedThunk<MonoObject*, MonoObject*> EntityOnCollishionEnd;
		UnmanagedThunk<MonoObject*, MonoObject*> EntityOnTriggerBegin;
		UnmanagedThunk<MonoObject*, MonoObject*> EntityOnTriggerEnd;
	};
	static MonoGlueData* s_ScriptGlue = nullptr;

	namespace utils {

		static Entity TryGetEntity(uint64_t id)
		{
			Ref<Scene> scene = ScriptEngine::GetActiveScene();
			return scene ? scene->GetEntityByUUID(id) : Entity{};
		}

		static Ref<Scene> GetScene()
		{
			return ScriptEngine::GetActiveScene();
		}

		static RigidBody2DComponent::BodyType b2BodyTypeToSharkBodyType(b2BodyType bodyType)
		{
			switch (bodyType)
			{
				case b2_staticBody:
					return RigidBody2DComponent::BodyType::Static;
				case b2_kinematicBody:
					return RigidBody2DComponent::BodyType::Kinematic;
				case b2_dynamicBody:
					return RigidBody2DComponent::BodyType::Dynamic;
			}
			SK_CORE_ASSERT(false, "Invalid Body Type");
			return RigidBody2DComponent::BodyType::Static;
		}

		static b2BodyType SharkBodyTypeTob2BodyType(RigidBody2DComponent::BodyType bodyType)
		{
			switch (bodyType)
			{
				case RigidBody2DComponent::BodyType::Static:
					return b2_staticBody;
				case RigidBody2DComponent::BodyType::Kinematic:
					return b2_kinematicBody;
				case RigidBody2DComponent::BodyType::Dynamic:
					return b2_dynamicBody;
			}
			SK_CORE_ASSERT(false, "Invalid Body Type");
			return b2_staticBody;
		}

		static MonoMethod* GetCoreMethod(const std::string& fullName)
		{
			MonoMethodDesc* methodDesc = mono_method_desc_new(fullName.c_str(), true);
			MonoMethod* method = mono_method_desc_search_in_image(methodDesc, ScriptEngine::GetCoreAssemblyInfo().Image);
			mono_method_desc_free(methodDesc);
			return method;
		}

		static const char* GetCollider2DTypeName(Collider2DType colliderType)
		{
			switch (colliderType)
			{
				case Collider2DType::BoxCollider:
					return "BoxCollider2DComponent";
				case Collider2DType::CircleCollider:
					return "CircleCollider2DComponent";
			}

			SK_CORE_ASSERT(false, "Unkown Collider2D type");
			return nullptr;
		}

		static MonoObject* CreateColliderObject(MonoObject* entity, Collider2DType colliderType)
		{
			const char* colliderName = colliderType == Collider2DType::BoxCollider ? "BoxCollider2DComponent" : "CircleCollider2DComponent";

			MonoClass* clazz = mono_class_from_name_case(ScriptEngine::GetCoreAssemblyInfo().Image, "Shark", colliderName);
			MonoObject* object = ScriptEngine::InstantiateClass(clazz);
			mono_runtime_object_init(object);

			MonoMethodDesc* ctorDesc = mono_method_desc_new(":.ctor()", false);
			MonoMethod* method = mono_method_desc_search_in_class(ctorDesc, clazz);
			mono_method_desc_free(ctorDesc);
			ScriptEngine::InvokeMethod(object, method);

			MonoProperty* property = mono_class_get_property_from_name(clazz, "Entity");
			void* params[] = { entity };
			MonoObject* exception = nullptr;
			mono_property_set_value(property, object, params, &exception);
			ScriptUtils::HandleException(exception);

			return object;
		}

		static MonoObject* GetOrCreateInstance(Entity entity)
		{
			if (ScriptEngine::IsInstantiated(entity))
			{
				GCHandle handle = ScriptEngine::GetInstance(entity);
				return GCManager::GetManagedObject(handle);
			}
			return ScriptEngine::InstantiateBaseEntity(entity);
		}

	}

	template <typename Component>
	static void RegisterComponent()
	{
		std::string_view fullName = typeid(Component).name();
		size_t separator = fullName.rfind(':');
		std::string_view name = fullName.substr(separator + 1);
		std::string managedComponentName = fmt::format("Shark.{}", name);

		MonoType* componentType = mono_reflection_type_from_name(managedComponentName.data(), ScriptEngine::GetCoreAssemblyInfo().Image);

		auto& bindings = s_EntityBindings[componentType];
		bindings.HasComponent = [](Entity entity) { return entity.AllOf<Component>(); };
		bindings.AddComponent = [](Entity entity) { entity.AddComponent<Component>(); };
		bindings.RemoveComponent = [](Entity entity) { entity.RemoveComponent<Component>(); };
	}

	void ScriptGlue::Init()
	{
		SK_PROFILE_FUNCTION();

		s_ScriptGlue = new MonoGlueData();

		s_ScriptGlue->EntityOnCollishionBegin = utils::GetCoreMethod("Shark.Entity:OnCollishionBegin");
		s_ScriptGlue->EntityOnCollishionEnd = utils::GetCoreMethod("Shark.Entity:OnCollishionEnd");
		s_ScriptGlue->EntityOnTriggerBegin = utils::GetCoreMethod("Shark.Entity:OnTriggerBegin");
		s_ScriptGlue->EntityOnTriggerEnd = utils::GetCoreMethod("Shark.Entity:OnTriggerEnd");

		SK_CORE_ASSERT(s_ScriptGlue->EntityOnCollishionBegin.Method);
		SK_CORE_ASSERT(s_ScriptGlue->EntityOnCollishionEnd.Method);
		SK_CORE_ASSERT(s_ScriptGlue->EntityOnTriggerBegin.Method);
		SK_CORE_ASSERT(s_ScriptGlue->EntityOnTriggerEnd.Method);

		RegisterComponents();
		RegisterInternalCalls();
	}

	void ScriptGlue::Shutdown()
	{
		delete s_ScriptGlue;
		s_ScriptGlue = nullptr;
		s_EntityBindings.clear();
	}

	static void CallPhyiscsFunc(UnmanagedThunk<MonoObject*, MonoObject*> method, Entity entityA, Entity entityB, Collider2DType colliderType)
	{
		if (!ScriptEngine::IsInstantiated(entityA))
			return;

		GCHandle objectHandle = ScriptEngine::GetInstance(entityA);
		MonoObject* object = GCManager::GetManagedObject(objectHandle);

		MonoObject* entity = utils::GetOrCreateInstance(entityB);
		MonoObject* collider = utils::CreateColliderObject(entity, colliderType);

		MonoException* exception = nullptr;
		method.Invoke(object, collider, &exception);
		ScriptUtils::HandleException((MonoObject*)exception);
	}

	void ScriptGlue::CallCollishionBegin(Entity entityA, Entity entityB, Collider2DType colliderType, bool isSensor)
	{
		if (!s_ScriptGlue)
			return;

		auto func = isSensor ? s_ScriptGlue->EntityOnTriggerBegin : s_ScriptGlue->EntityOnCollishionBegin;
		CallPhyiscsFunc(func, entityA, entityB, colliderType);
	}

	void ScriptGlue::CallCollishionEnd(Entity entityA, Entity entityB, Collider2DType colliderType, bool isSensor)
	{
		if (!s_ScriptGlue)
			return;

		auto func = isSensor ? s_ScriptGlue->EntityOnTriggerEnd : s_ScriptGlue->EntityOnCollishionEnd;
		CallPhyiscsFunc(func, entityA, entityB, colliderType);
	}

	void ScriptGlue::RegisterComponents()
	{
		SK_PROFILE_FUNCTION();

		RegisterComponent<IDComponent>();
		RegisterComponent<TagComponent>();
		RegisterComponent<TransformComponent>();
		RegisterComponent<SpriteRendererComponent>();
		RegisterComponent<CircleRendererComponent>();
		RegisterComponent<CameraComponent>();
		RegisterComponent<RigidBody2DComponent>();
		RegisterComponent<BoxCollider2DComponent>();
		RegisterComponent<CircleCollider2DComponent>();
	}

	void ScriptGlue::RegisterInternalCalls()
	{
		SK_PROFILE_FUNCTION();

		SK_ADD_INTERNAL_CALL(Application_GetWidth);
		SK_ADD_INTERNAL_CALL(Application_GetHeight);

		SK_ADD_INTERNAL_CALL(Log_LogMessage);

		SK_ADD_INTERNAL_CALL(Input_IsKeyStateSet);
		SK_ADD_INTERNAL_CALL(Input_IsMouseStateSet);
		SK_ADD_INTERNAL_CALL(Input_GetMouseScroll);
		SK_ADD_INTERNAL_CALL(Input_GetMousePos);

		SK_ADD_INTERNAL_CALL(Matrix4_Inverse);
		SK_ADD_INTERNAL_CALL(Matrix4_Matrix4MulMatrix4);
		SK_ADD_INTERNAL_CALL(Matrix4_Matrix4MulVector4);

		SK_ADD_INTERNAL_CALL(Entity_GetInstance);
		SK_ADD_INTERNAL_CALL(Entity_HasParent);
		SK_ADD_INTERNAL_CALL(Entity_GetParent);
		SK_ADD_INTERNAL_CALL(Entity_GetChildren);
		SK_ADD_INTERNAL_CALL(Entity_HasComponent);
		SK_ADD_INTERNAL_CALL(Entity_AddComponent);
		SK_ADD_INTERNAL_CALL(Entity_Instantiate);
		SK_ADD_INTERNAL_CALL(Entity_DestroyEntity);
		SK_ADD_INTERNAL_CALL(Entity_CreateEntity);
		SK_ADD_INTERNAL_CALL(Entity_CloneEntity);
		SK_ADD_INTERNAL_CALL(Entity_FindEntityByName);
		SK_ADD_INTERNAL_CALL(Entity_FindChildEntityByName);

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

		SK_ADD_INTERNAL_CALL(Physics2D_GetGravity);
		SK_ADD_INTERNAL_CALL(Physics2D_SetGravity);

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

	namespace InternalCalls
	{

		#pragma region Application

		uint32_t Application_GetWidth()
		{
			Ref<Scene> scene = utils::GetScene();
			SK_CORE_ASSERT(scene);
			if (!scene)
			{
				MonoException* exception = mono_get_exception_null_reference();
				mono_raise_exception(exception);
				SK_CORE_ASSERT(false, "should not be called");
				return 0;
			}
			return scene->GetViewportWidth();
		}

		uint32_t Application_GetHeight()
		{
			Ref<Scene> scene = utils::GetScene();
			SK_CORE_ASSERT(scene);
			if (!scene)
			{
				MonoException* exception = mono_get_exception_null_reference();
				mono_raise_exception(exception);
				SK_CORE_ASSERT(false, "should not be called");
				return 0;
			}
			return scene->GetViewportHeight();
		}

		#pragma endregion

		#pragma region Log

		void Log_LogMessage(LogLevelType level, MonoString* message)
		{
			char* msg = mono_string_to_utf8(message);
			switch (level)
			{
				case LogLevelType::Trace:
					SK_CONSOLE_TRACE(msg ? msg : "Message was Null");
					break;
				case LogLevelType::Info:
					SK_CONSOLE_INFO(msg ? msg : "Message was Null");
					break;
				case LogLevelType::Warn:
					SK_CONSOLE_WARN(msg ? msg : "Message was Null");
					break;
				case LogLevelType::Error:
					SK_CONSOLE_ERROR(msg ? msg : "Message was Null");
					break;
				case LogLevelType::Critical:
					SK_CONSOLE_CRITICAL(msg ? msg : "Message was Null");
					break;
				case LogLevelType::Debug:
					SK_CONSOLE_DEBUG(msg ? msg : "Message was Null");
					break;
			}
			mono_free(msg);
		}

		#pragma endregion

		#pragma region Input

		bool Input_IsKeyStateSet(KeyCode key, KeyState keyState)
		{
			auto& app = Application::Get();
			if (app.GetSpecs().EnableImGui)
			{
				const ImGuiLayer& imguiLayer = app.GetImGuiLayer();
				if (imguiLayer.BlocksKeyboardEvents())
					return false;
			}

			return Input::GetKeyState(key) == keyState;
		}

		bool Input_IsMouseStateSet(MouseButton button, MouseState mouseState)
		{
			auto& app = Application::Get();
			if (app.GetSpecs().EnableImGui)
			{
				const ImGuiLayer& imguiLayer = app.GetImGuiLayer();
				if (imguiLayer.BlocksMouseEvents())
					return false;
			}

			return Input::GetMouseState(button) == mouseState;
		}

		float Input_GetMouseScroll()
		{
			auto& app = Application::Get();
			if (app.GetSpecs().EnableImGui)
			{
				const ImGuiLayer& imguiLayer = app.GetImGuiLayer();
				if (imguiLayer.BlocksMouseEvents())
					return false;
			}

			return Input::GetMouseScroll();
		}

		void Input_GetMousePos(glm::ivec2* out_MousePos)
		{
			auto p = Input::GetScreenMousePosition();

			Application& app = Application::Get();
			if (app.GetSpecs().EnableImGui)
			{
				ImGuiWindow* viewportWindow = ImGui::FindWindowByID(app.GetImGuiLayer().GetMainViewportID());
				if (viewportWindow)
				{
					p.x -= (int)viewportWindow->Pos.x;
					p.y -= (int)viewportWindow->Pos.y;
				}
			}

			*out_MousePos = { p.x, p.y };
		}

		#pragma endregion

		#pragma region Matrix4
 
		void Matrix4_Inverse(glm::mat4* matrix, glm::mat4* out_Result)
		{
			*out_Result = glm::inverse(*matrix);
		}

		void Matrix4_Matrix4MulMatrix4(glm::mat4* lhs, glm::mat4* rhs, glm::mat4* out_Result)
		{
			*out_Result = *lhs * *rhs;
		}

		void Matrix4_Matrix4MulVector4(glm::mat4* lhs, glm::vec4* rhs, glm::vec4* out_Result)
		{
			*out_Result = *lhs * *rhs;
		}

		#pragma endregion

		#pragma region Entity

		MonoObject* Entity_GetInstance(uint64_t entityID)
		{
			Entity entity = utils::TryGetEntity(entityID);
			if (!entity)
				return false;

			if (ScriptEngine::IsInstantiated(entity))
			{
				GCHandle handle = ScriptEngine::GetInstance(entity);
				return GCManager::GetManagedObject(handle);
			}
			return nullptr;
		}

		bool Entity_HasParent(uint64_t entityID)
		{
			Entity entity = utils::TryGetEntity(entityID);
			if (!entity)
				return false;

			return entity.HasParent();
		}

		MonoObject* Entity_GetParent(uint64_t entityID)
		{
			Entity entity = utils::TryGetEntity(entityID);
			if (!entity)
				return nullptr;

			Entity parent = entity.Parent();
			if (!parent)
				return nullptr;

			return utils::GetOrCreateInstance(parent);
		}

		MonoArray* Entity_GetChildren(uint64_t entityID)
		{
			Entity entity = utils::TryGetEntity(entityID);
			if (!entity)
				return nullptr;

			Ref<Scene> scene = utils::GetScene();
			std::vector<UUID> children = entity.Children();
			MonoArray* arr = mono_array_new(ScriptEngine::GetRuntimeDomain(), ScriptEngine::GetEntityClass(), children.size());
			for (uint32_t i = 0; i < children.size(); i++)
			{
				Entity child = scene->GetEntityByUUID(children[i]);
				MonoObject* obj = utils::GetOrCreateInstance(child);
				mono_array_setref(arr, i, obj);
			}

			return arr;
		}

		bool Entity_HasComponent(uint64_t id, MonoReflectionType* type)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return false;

			MonoType* componentType = mono_reflection_type_get_type(type);

			if (s_EntityBindings.find(componentType) == s_EntityBindings.end())
				return false;

			auto& bindings = s_EntityBindings.at(componentType);
			return bindings.HasComponent(entity);
		}

		void Entity_AddComponent(uint64_t id, MonoReflectionType* type)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;

			MonoType* componentType = mono_reflection_type_get_type(type);

			if (s_EntityBindings.find(componentType) == s_EntityBindings.end())
				return;

			auto& bindings = s_EntityBindings.at(componentType);
			if (bindings.HasComponent(entity))
				return;

			bindings.AddComponent(entity);
		}

		void Entity_RemoveComponent(uint64_t id, MonoReflectionType* type)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;

			MonoType* componentType = mono_reflection_type_get_type(type);

			if (s_EntityBindings.find(componentType) == s_EntityBindings.end())
				return;

			auto& bindings = s_EntityBindings.at(componentType);
			if (!bindings.HasComponent(entity))
				return;

			bindings.RemoveComponent(entity);
		}

		MonoObject* Entity_Instantiate(MonoReflectionType* type, MonoString* name)
		{
			MonoType* monoType = mono_reflection_type_get_type(type);
			char* scriptTypeName = mono_type_get_name(monoType);

			Ref<ScriptClass> klass = ScriptEngine::GetScriptClassFromName(scriptTypeName);
			if (!klass)
			{
				SK_CONSOLE_ERROR("Can't Instantiate Entity\nScript type not found");
				return nullptr;
			}

			Ref<Scene> scene = utils::GetScene();
			Entity newEntity = scene->CreateEntity(ScriptUtils::MonoStringToUTF8(name));
			auto& comp = newEntity.AddComponent<ScriptComponent>();
			comp.ScriptName = scriptTypeName;
			comp.ClassID = klass->GetID();
			mono_free(scriptTypeName);

			GCHandle handle = ScriptEngine::InstantiateEntity(newEntity, true, false);
			return handle ? GCManager::GetManagedObject(handle) : nullptr;
		}

		void Entity_DestroyEntity(uint64_t entityID, bool destroyChildren)
		{
			Ref<Scene> scene = utils::GetScene();
			scene->Submit([scene, entityID, destroyChildren]()
			{
				Entity entity = scene->GetEntityByUUID(entityID);
				if (!entity)
					return;

				scene->DestroyEntity(entity, destroyChildren);
			});
		}

		uint64_t Entity_CreateEntity(MonoString* name)
		{
			Ref<Scene> scene = utils::GetScene();
			if (!scene)
				return 0;

			std::string entityName = ScriptUtils::MonoStringToUTF8(name);
			Entity entity = scene->CreateEntity(entityName);
			return entity.GetUUID();
		}

		uint64_t Entity_CloneEntity(uint64_t entityID)
		{
			Ref<Scene> scene = utils::GetScene();
			if (!scene)
				return 0;

			Entity entity = scene->GetEntityByUUID(entityID);
			if (!entity)
				return 0;

			Entity clonedEntity = scene->CloneEntity(entity);
			ScriptEngine::OnEntityCloned(entity, clonedEntity);
			return clonedEntity.GetUUID();
		}

		uint64_t Entity_FindEntityByName(MonoString* name)
		{
			Ref<Scene> scene = utils::GetScene();
			if (!scene)
				return 0;

			std::string entityTag = ScriptUtils::MonoStringToUTF8(name);
			Entity entity = scene->FindEntityByTag(entityTag);
			if (entity)
				return entity.GetUUID();
			return 0;
		}

		uint64_t Entity_FindChildEntityByName(uint64_t entityID, MonoString* name, bool recusive)
		{
			Ref<Scene> scene = utils::GetScene();
			if (!scene)
				return 0;

			Entity entity = scene->GetEntityByUUID(entityID);
			if (!entity)
				return 0;

			std::string entityName = ScriptUtils::MonoStringToUTF8(name);
			Entity childEntity = scene->FindChildEntityByName(entity, entityName, recusive);
			if (childEntity)
				return childEntity.GetUUID();
			return 0;
		}

#pragma endregion

		#pragma region TagComponent

		MonoString* TagComponent_GetTag(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return ScriptUtils::MonoStringEmpty();

			return ScriptUtils::UTF8ToMonoString(entity.GetName());
		}

		void TagComponent_SetTag(uint64_t id, MonoString* tag)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;

			entity.GetComponent<TagComponent>().Tag = ScriptUtils::MonoStringToUTF8(tag);
		}

		#pragma endregion

		#pragma region TransformComponent

		void TransformComponent_GetTranslation(uint64_t id, glm::vec3* out_Translation)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;

			auto& transform = entity.Transform();
			*out_Translation = transform.Translation;
		}

		void TransformComponent_SetTranslation(uint64_t id, glm::vec3* translation)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;

			auto& transform = entity.Transform();
			transform.Translation = *translation;
		}

		void TransformComponent_GetRotation(uint64_t id, glm::vec3* out_Rotation)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;

			auto& transform = entity.Transform();
			*out_Rotation = transform.Rotation;
		}

		void TransformComponent_SetRotation(uint64_t id, glm::vec3* rotation)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;

			auto& transform = entity.Transform();
			transform.Rotation = *rotation;
		}

		void TransformComponent_GetScale(uint64_t id, glm::vec3* out_Scaling)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;

			auto& transform = entity.Transform();
			*out_Scaling = transform.Scale;
		}

		void TransformComponent_SetScale(uint64_t id, glm::vec3* scaling)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;

			auto& transform = entity.Transform();
			transform.Scale = *scaling;
		}

		void TransformComponent_GetLocalTransform(uint64_t id, TransformComponent* out_LocalTransform)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;

			*out_LocalTransform = entity.Transform();
		}

		void TransformComponent_SetLocalTransform(uint64_t id, TransformComponent* localTransform)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;

			entity.Transform() = *localTransform;
		}

		void TransformComponent_GetWorldTransform(uint64_t id, TransformComponent* out_WorldTransform)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;

			if (!entity.HasParent())
			{
				*out_WorldTransform = entity.Transform();
				return;
			}

			Ref<Scene> scene = utils::GetScene();
			glm::mat4 worldTransform = scene->GetWorldSpaceTransformMatrix(entity);
			Math::DecomposeTransform(worldTransform, out_WorldTransform->Translation, out_WorldTransform->Rotation, out_WorldTransform->Scale);
		}

		void TransformComponent_SetWorldTransform(uint64_t id, TransformComponent* worldTransform)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;

			if (!entity.HasParent())
			{
				entity.Transform() = *worldTransform;
				return;
			}

			Ref<Scene> scene = utils::GetScene();

			glm::mat4 transform = worldTransform->CalcTransform();
			glm::mat4 parentTransform = entity.Parent().CalcTransform();

			glm::mat4 localTransform = glm::inverse(parentTransform) * transform;

			TransformComponent& tf = entity.Transform();
			Math::DecomposeTransform(localTransform, tf.Translation, tf.Rotation, tf.Scale);
		}

		#pragma endregion

		#pragma region SpriteRendererComponent

		void SpriteRendererComponent_GetColor(uint64_t id, glm::vec4* out_Color)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<SpriteRendererComponent>())
				return;

			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			*out_Color = spriteRenderer.Color;
		}

		void SpriteRendererComponent_SetColor(uint64_t id, glm::vec4* color)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<SpriteRendererComponent>())
				return;

			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			spriteRenderer.Color = *color;
		}

		AssetHandle SpriteRendererComponent_GetTextureHandle(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return AssetHandle::Null;
			if (!entity.AllOf<SpriteRendererComponent>())
				return AssetHandle::Null;

			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			return spriteRenderer.TextureHandle;
		}

		void SpriteRendererComponent_SetTextureHandle(uint64_t id, AssetHandle textureHandle)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<SpriteRendererComponent>())
				return;

			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			spriteRenderer.TextureHandle = textureHandle;
		}

		float SpriteRendererComponent_GetTilingFactor(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<SpriteRendererComponent>())
				return 0.0f;

			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			return spriteRenderer.TilingFactor;
		}

		void SpriteRendererComponent_SetTilingFactor(uint64_t id, float tilingFactor)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<SpriteRendererComponent>())
				return;

			auto& spriteRenderer = entity.GetComponent<SpriteRendererComponent>();
			spriteRenderer.TilingFactor = tilingFactor;
		}

		#pragma endregion

		#pragma region CricleRendererComponent

		void CircleRendererComponent_GetColor(uint64_t id, glm::vec4* out_Color)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CircleRendererComponent>())
				return;

			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			*out_Color = circleRenderer.Color;
		}

		void CircleRendererComponent_SetColor(uint64_t id, glm::vec4* color)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CircleRendererComponent>())
				return;

			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			circleRenderer.Color = *color;
		}

		float CircleRendererComponent_GetThickness(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CircleRendererComponent>())
				return 0.0f;

			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			return circleRenderer.Thickness;
		}

		void CircleRendererComponent_SetThickness(uint64_t id, float thickness)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CircleRendererComponent>())
				return;

			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			circleRenderer.Thickness = thickness;
		}

		float CircleRendererComponent_GetFade(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CircleRendererComponent>())
				return 0.0f;

			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			return circleRenderer.Fade;
		}

		void CircleRendererComponent_SetFade(uint64_t id, float fade)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CircleRendererComponent>())
				return;

			auto& circleRenderer = entity.GetComponent<CircleRendererComponent>();
			circleRenderer.Fade = fade;
		}

		#pragma endregion

		#pragma region CameraComponent

		void CameraComponent_GetProjection(uint64_t id, glm::mat4* out_Projection)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CameraComponent>())
				return;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			*out_Projection = camera.GetProjection();
		}

		void CameraComponent_SetProjection(uint64_t id, glm::mat4* projection)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CameraComponent>())
				return;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetProjection(*projection);
		}

		SceneCamera::Projection CameraComponent_GetProjectionType(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return SceneCamera::Projection::None;
			if (!entity.AllOf<CameraComponent>())
				return SceneCamera::Projection::None;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetProjectionType();
		}

		void CameraComponent_SetProjectionType(uint64_t id, SceneCamera::Projection projectionType)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CameraComponent>())
				return;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetProjectionType(projectionType);
		}

		void CameraComponent_SetPerspective(uint64_t id, float aspectratio, float fov, float clipnear, float clipfar)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CameraComponent>())
				return;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspective(aspectratio, fov, clipnear, clipfar);
		}

		void CameraComponent_SetOrthographic(uint64_t id, float aspectratio, float zoom, float clipnear, float clipfar)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CameraComponent>())
				return;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographic(aspectratio, zoom, clipnear, clipfar);
		}

		float CameraComponent_GetAspectratio(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CameraComponent>())
				return 0.0f;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetAspectratio();
		}

		void CameraComponent_SetAspectratio(uint64_t id, float aspectratio)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CameraComponent>())
				return;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetAspectratio(aspectratio);
		}

		float CameraComponent_GetPerspectiveFOV(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CameraComponent>())
				return 0.0f;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetPerspectiveFOV();
		}

		void CameraComponent_SetPerspectiveFOV(uint64_t id, float fov)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CameraComponent>())
				return;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspectiveFOV(fov);
		}

		float CameraComponent_GetPerspectiveNear(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CameraComponent>())
				return 0.0f;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetPerspectiveNear();
		}

		void CameraComponent_SetPerspectiveNear(uint64_t id, float clipnear)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CameraComponent>())
				return;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspectiveNear(clipnear);
		}

		float CameraComponent_GetPerspectiveFar(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CameraComponent>())
				return 0.0f;
			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetPerspectiveFar();
		}

		void CameraComponent_SetPerspectiveFar(uint64_t id, float clipfar)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CameraComponent>())
				return;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetPerspectiveFar(clipfar);
		}

		float CameraComponent_GetOrthographicZoom(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CameraComponent>())
				return 0.0f;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetOrthographicZoom();
		}

		void CameraComponent_SetOrthographicZoom(uint64_t id, float zoom)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CameraComponent>())
				return;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographicZoom(zoom);
		}

		float CameraComponent_GetOrthographicNear(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CameraComponent>())
				return 0.0f;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetOrthographicNear();
		}

		void CameraComponent_SetOrthographicNear(uint64_t id, float clipnear)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CameraComponent>())
				return;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographicNear(clipnear);
		}

		float CameraComponent_GetOrthographicFar(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CameraComponent>())
				return 0.0f;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			return camera.GetOrthographicFar();
		}

		void CameraComponent_SetOrthographicFar(uint64_t id, float clipfar)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CameraComponent>())
				return;

			auto& camera = entity.GetComponent<CameraComponent>().Camera;
			camera.SetOrthographicFar(clipfar);
		}

		#pragma endregion

		#pragma region Physics2D

		void Physics2D_GetGravity(glm::vec2* out_Gravity)
		{
			Ref<Scene> scene = utils::GetScene();
			auto& phyiscsScene = scene->GetPhysicsScene();
			if (!phyiscsScene.GetWorld())
			{
				*out_Gravity = glm::vec2(0.0f);
				return;
			}

			auto gravity = phyiscsScene.GetWorld()->GetGravity();
			out_Gravity->x = gravity.x;
			out_Gravity->y = gravity.y;
		}

		void Physics2D_SetGravity(glm::vec2* gravity)
		{
			Ref<Scene> scene = utils::GetScene();
			auto& phyiscsScene = scene->GetPhysicsScene();
			if (!phyiscsScene.GetWorld())
				return;

			phyiscsScene.GetWorld()->SetGravity({ gravity->x, gravity->y });
		}

		#pragma endregion

		#pragma region RigidBody2DComponent

		RigidBody2DComponent::BodyType RigidBody2DComponent_GetBodyType(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return RigidBody2DComponent::BodyType::None;
			if (!entity.AllOf<RigidBody2DComponent>())
				return RigidBody2DComponent::BodyType::None;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return RigidBody2DComponent::BodyType::None;

			return utils::b2BodyTypeToSharkBodyType(body->GetType());
		}

		void RigidBody2DComponent_SetBodyType(uint64_t id, RigidBody2DComponent::BodyType bodyType)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			body->SetType(utils::SharkBodyTypeTob2BodyType(bodyType));
		}

		void RigidBody2DComponent_GetTransform(uint64_t id, RigidBody2DTransform* out_Transform)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			const auto& tf = body->GetTransform();

			RigidBody2DTransform transform;
			transform.Position = { tf.p.x, tf.p.y };
			transform.Angle = tf.q.GetAngle();
			*out_Transform = transform;
		}

		void RigidBody2DComponent_SetTransform(uint64_t id, RigidBody2DTransform* transform)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			const b2Vec2 pos = { transform->Position.x, transform->Position.y };
			body->SetTransform(pos, transform->Angle);
		}

		void RigidBody2DComponent_SetPosition(uint64_t id, glm::vec2* position)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			body->SetTransform({ position->x, position->y }, body->GetAngle());
		}

		void RigidBody2DComponent_SetRotation(uint64_t id, float rotation)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			body->SetTransform(body->GetPosition(), rotation);
		}

		void RigidBody2DComponent_GetLocalCenter(uint64_t id, glm::vec2* out_LocalCenter)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			const b2Vec2 lc = body->GetLocalCenter();
			*out_LocalCenter = { lc.x, lc.y };
		}

		void RigidBody2DComponent_GetWorldCenter(uint64_t id, glm::vec2* out_WorldCenter)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			const b2Vec2 wc = body->GetWorldCenter();
			*out_WorldCenter = { wc.x, wc.y };
		}

		void RigidBody2DComponent_GetLinearVelocity(uint64_t id, glm::vec2* out_LinearVelocity)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			const b2Vec2& lv = body->GetLinearVelocity();
			*out_LinearVelocity = { lv.x, lv.y };
		}

		void RigidBody2DComponent_SetLinearVelocity(uint64_t id, glm::vec2* linearVelocity)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			body->SetLinearVelocity({ linearVelocity->x, linearVelocity->y });
		}

		float RigidBody2DComponent_GetAngularVelocity(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<RigidBody2DComponent>())
				return 0.0f;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return 0.0f;

			return body->GetAngularVelocity();
		}

		void RigidBody2DComponent_SetAngularVelocity(uint64_t id, float angularVelocity)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			body->SetAngularVelocity(angularVelocity);
		}

		void RigidBody2DComponent_ApplyForce(uint64_t id, glm::vec2* force, glm::vec2* point, PhysicsForce2DType forceType)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			if (forceType == PhysicsForce2DType::Force)
				body->ApplyForce({ force->x, force->y }, { point->x, point->y }, true);
			else
				body->ApplyLinearImpulse({ force->x, force->y }, { point->x, point->y }, true);
		}

		void RigidBody2DComponent_ApplyForceToCenter(uint64_t id, glm::vec2* force, PhysicsForce2DType forceType)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			if (forceType == PhysicsForce2DType::Force)
				body->ApplyForceToCenter({ force->x, force->y }, true);
			else
				body->ApplyLinearImpulseToCenter({ force->x, force->y }, true);
		}

		void RigidBody2DComponent_ApplyTorque(uint64_t id, float torque, PhysicsForce2DType forceType)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			if (forceType == PhysicsForce2DType::Force)
				body->ApplyTorque(torque, true);
			else
				body->ApplyAngularImpulse(torque, true);
		}

		float RigidBody2DComponent_GetGravityScale(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<RigidBody2DComponent>())
				return 0.0f;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return 0.0f;

			return body->GetGravityScale();
		}

		void RigidBody2DComponent_SetGravityScale(uint64_t id, float gravityScale)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			comp.GravityScale = gravityScale;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			body->SetGravityScale(gravityScale);
		}

		float RigidBody2DComponent_GetLinearDamping(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<RigidBody2DComponent>())
				return 0.0f;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return 0.0f;

			return body->GetLinearDamping();
		}

		void RigidBody2DComponent_SetLinearDamping(uint64_t id, float linearDamping)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			body->SetLinearDamping(linearDamping);
		}

		float RigidBody2DComponent_GetAngularDamping(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<RigidBody2DComponent>())
				return 0.0f;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return 0.0f;

			return body->GetAngularDamping();
		}

		void RigidBody2DComponent_SetAngularDamping(uint64_t id, float angularDamping)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			body->SetAngularDamping(angularDamping);
		}

		bool RigidBody2DComponent_IsBullet(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return false;
			if (!entity.AllOf<RigidBody2DComponent>())
				return false;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			return body->IsBullet();
		}

		void RigidBody2DComponent_SetBullet(uint64_t id, bool bullet)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			comp.IsBullet = bullet;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			body->SetBullet(bullet);
		}

		bool RigidBody2DComponent_IsSleepingAllowed(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return false;
			if (!entity.AllOf<RigidBody2DComponent>())
				return false;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			return body->IsSleepingAllowed();
		}

		void RigidBody2DComponent_SetSleepingAllowed(uint64_t id, bool sleepingAllowed)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			comp.AllowSleep = sleepingAllowed;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			body->SetSleepingAllowed(sleepingAllowed);
		}

		bool RigidBody2DComponent_IsAwake(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return false;
			if (!entity.AllOf<RigidBody2DComponent>())
				return false;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			return body->IsAwake();
		}

		void RigidBody2DComponent_SetAwake(uint64_t id, bool awake)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			comp.Awake = awake;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			body->SetAwake(awake);
		}

		bool RigidBody2DComponent_IsEnabled(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return false;
			if (!entity.AllOf<RigidBody2DComponent>())
				return false;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			return body->IsEnabled();
		}

		void RigidBody2DComponent_SetEnabled(uint64_t id, bool enabled)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			comp.Enabled = enabled;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			body->SetEnabled(enabled);
		}

		bool RigidBody2DComponent_IsFixedRotation(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return false;
			if (!entity.AllOf<RigidBody2DComponent>())
				return false;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return false;

			return body->IsFixedRotation();
		}

		void RigidBody2DComponent_SetFixedRotation(uint64_t id, bool fixedRotation)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<RigidBody2DComponent>())
				return;

			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			comp.FixedRotation = fixedRotation;

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			if (!body)
				return;

			body->SetFixedRotation(fixedRotation);
		}

		#pragma endregion

		#pragma region BoxCollider2DComponent

		void BoxCollider2DComponent_SetSensor(uint64_t id, bool sensor)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			comp.IsSensor = sensor;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return;

			fixture->SetSensor(sensor);
		}

		bool BoxCollider2DComponent_IsSensor(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return false;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return false;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			return fixture->IsSensor();
		}

		void BoxCollider2DComponent_SetDensity(uint64_t id, float density)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			comp.Density = density;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return;

			fixture->SetDensity(density);
		}

		float BoxCollider2DComponent_GetDensity(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return 0.0f;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return 0.0f;

			return fixture->GetDensity();
		}

		void BoxCollider2DComponent_SetFriction(uint64_t id, float friction)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			comp.Friction = friction;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return;

			fixture->SetFriction(friction);
		}

		float BoxCollider2DComponent_GetFriction(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return 0.0f;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return 0.0f;

			return fixture->GetFriction();
		}

		void BoxCollider2DComponent_SetRestitution(uint64_t id, float restitution)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			comp.Restitution = restitution;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return;

			fixture->SetRestitution(restitution);
		}

		float BoxCollider2DComponent_GetRestitution(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return 0.0f;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return 0.0f;

			return fixture->GetRestitution();
		}

		void BoxCollider2DComponent_SetRestitutionThreshold(uint64_t id, float restitutionThreshold)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			comp.RestitutionThreshold = restitutionThreshold;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return;

			fixture->SetRestitutionThreshold(restitutionThreshold);
		}

		float BoxCollider2DComponent_GetRestitutionThreshold(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return 0.0f;

			b2Fixture* fixture = entity.GetComponent<BoxCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return 0.0f;

			return fixture->GetRestitutionThreshold();
		}

		void BoxCollider2DComponent_GetSize(uint64_t id, glm::vec2* out_Size)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			*out_Size = comp.Size;
		}

		void BoxCollider2DComponent_SetSize(uint64_t id, glm::vec2* size)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			const auto& transform = entity.Transform();
			comp.Size = *size;

			if (!comp.RuntimeCollider)
				return;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_polygon);
			b2PolygonShape* shape = (b2PolygonShape*)comp.RuntimeCollider->GetShape();
			shape->SetAsBox(comp.Size.x * transform.Scale.x, comp.Size.y * transform.Scale.y, { comp.Offset.x, comp.Offset.y }, comp.Rotation);
		}

		void BoxCollider2DComponent_GetOffset(uint64_t id, glm::vec2* out_Offset)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			*out_Offset = comp.Offset;
		}

		void BoxCollider2DComponent_SetOffset(uint64_t id, glm::vec2* offset)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			const auto& transform = entity.Transform();
			comp.Offset = *offset;

			if (!comp.RuntimeCollider)
				return;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_polygon);
			b2PolygonShape* shape = (b2PolygonShape*)comp.RuntimeCollider->GetShape();
			shape->SetAsBox(comp.Size.x * transform.Scale.x, comp.Size.y * transform.Scale.y, { comp.Offset.x, comp.Offset.y }, comp.Rotation);
		}

		float BoxCollider2DComponent_GetRotation(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return 0.0f;

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			return comp.Rotation;
		}

		void BoxCollider2DComponent_SetRotation(uint64_t id, float rotation)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<BoxCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<BoxCollider2DComponent>();
			const auto& transform = entity.Transform();
			comp.Rotation = rotation;

			if (!comp.RuntimeCollider)
				return;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_polygon);
			b2PolygonShape* shape = (b2PolygonShape*)comp.RuntimeCollider->GetShape();
			shape->SetAsBox(comp.Size.x * transform.Scale.x, comp.Size.y * transform.Scale.y, { comp.Offset.x, comp.Offset.y }, comp.Rotation);
		}

		#pragma endregion

		#pragma region CircleCollider2DComponent

		void CircleCollider2DComponent_SetSensor(uint64_t id, bool sensor)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			comp.IsSensor = sensor;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return;

			fixture->SetSensor(sensor);
		}

		bool CircleCollider2DComponent_IsSensor(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return false;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return false;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return false;

			return fixture->IsSensor();
		}

		void CircleCollider2DComponent_SetDensity(uint64_t id, float density)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			comp.Density = density;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return;

			fixture->SetDensity(density);
		}

		float CircleCollider2DComponent_GetDensity(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return 0.0f;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return 0.0f;

			return fixture->GetDensity();
		}

		void CircleCollider2DComponent_SetFriction(uint64_t id, float friction)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			comp.Friction = friction;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return;

			fixture->SetFriction(friction);
		}

		float CircleCollider2DComponent_GetFriction(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return 0.0f;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return 0.0f;

			return fixture->GetFriction();
		}

		void CircleCollider2DComponent_SetRestitution(uint64_t id, float restitution)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			comp.Restitution = restitution;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return;

			fixture->SetRestitution(restitution);
		}

		float CircleCollider2DComponent_GetRestitution(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return 0.0f;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return 0.0f;

			return fixture->GetRestitution();
		}

		void CircleCollider2DComponent_SetRestitutionThreshold(uint64_t id, float restitutionThreshold)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			comp.RestitutionThreshold = restitutionThreshold;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return;

			fixture->SetRestitutionThreshold(restitutionThreshold);
		}

		float CircleCollider2DComponent_GetRestitutionThreshold(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return 0.0f;

			b2Fixture* fixture = entity.GetComponent<CircleCollider2DComponent>().RuntimeCollider;
			if (!fixture)
				return 0.0f;

			return fixture->GetRestitutionThreshold();
		}

		float CircleCollider2DComponent_GetRadius(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return 0.0f;

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			return comp.Radius;
		}

		void CircleCollider2DComponent_SetRadius(uint64_t id, float Radius)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			const auto& transform = entity.Transform();
			comp.Radius = Radius;

			if (!comp.RuntimeCollider)
				return;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_circle);
			b2CircleShape* shape = (b2CircleShape*)comp.RuntimeCollider->GetShape();
			shape->m_radius = comp.Radius * transform.Scale.x;
			shape->m_p = { comp.Offset.x, comp.Offset.y };
		}

		void CircleCollider2DComponent_GetOffset(uint64_t id, glm::vec2* out_Offsets)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			*out_Offsets = comp.Offset;
		}

		void CircleCollider2DComponent_SetOffset(uint64_t id, glm::vec2* offset)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			const auto& transform = entity.Transform();
			comp.Offset = *offset;

			if (!comp.RuntimeCollider)
				return;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_circle);
			b2CircleShape* shape = (b2CircleShape*)comp.RuntimeCollider->GetShape();
			shape->m_radius = comp.Radius * transform.Scale.x;
			shape->m_p = { comp.Offset.x, comp.Offset.y };
		}

		float CircleCollider2DComponent_GetRotation(uint64_t id)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return 0.0f;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return 0.0f;

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			return comp.Rotation;
		}

		void CircleCollider2DComponent_SetRotation(uint64_t id, float rotation)
		{
			Entity entity = utils::TryGetEntity(id);
			if (!entity)
				return;
			if (!entity.AllOf<CircleCollider2DComponent>())
				return;

			auto& comp = entity.GetComponent<CircleCollider2DComponent>();
			const auto& transform = entity.Transform();
			comp.Rotation = rotation;

			if (!comp.RuntimeCollider)
				return;

			SK_CORE_ASSERT(comp.RuntimeCollider->GetType() == b2Shape::e_circle);
			b2CircleShape* shape = (b2CircleShape*)comp.RuntimeCollider->GetShape();
			shape->m_radius = comp.Radius * transform.Scale.x;
			shape->m_p = { comp.Offset.x, comp.Offset.y };
		}

		#pragma endregion

		#pragma region ResoureManager

		void ResourceManager_GetAssetHandleFromFilePath(MonoString* filePath, AssetHandle* out_AssetHandle)
		{
			std::string path = ScriptUtils::MonoStringToUTF8(filePath);
			*out_AssetHandle = ResourceManager::GetAssetHandleFromFilePath(path);
		}

		#pragma endregion

		#pragma region EditorUI

		bool EditorUI_BeginWindow(MonoString* windowTitle)
		{
			auto& app = Application::Get();
			if (!(app.GetSpecs().EnableImGui && app.GetImGuiLayer().InFrame()))
				return false;

			char* cWindowTitle = mono_string_to_utf8(windowTitle);
			bool open = ImGui::Begin(cWindowTitle, nullptr, ImGuiWindowFlags_NoSavedSettings);
			mono_free(cWindowTitle);
			return open;
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
			mono_free(cText);
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
