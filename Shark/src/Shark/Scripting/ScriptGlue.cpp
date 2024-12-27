#include "skpch.h"
#include "ScriptGlue.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Log.h"
#include "Shark/Core/Project.h"
#include "Shark/Asset/AssetManager.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"
#include "Shark/Scene/Prefab.h"

#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Math/Math.h"

#include "Shark/Debug/Profiler.h"

#include <box2d/b2_body.h>
#include <box2d/b2_contact.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>

#include <imgui_internal.h>

namespace Shark {

	static std::map<Coral::TypeId, bool(*)(Entity)> s_HasComponentFunctions;
	static std::map<Coral::TypeId, void(*)(Entity)> s_AddComponentFunctions;
	static std::map<Coral::TypeId, void(*)(Entity)> s_RemoveComponentFunctions;

#define SK_ICALL_VERIFY_PARAMETER(_param) if (!(_param)) { SK_CONSOLE_ERROR("{} called with with invalid value for parameter '{}'", SK_FUNCTION_NAME, #_param); }
#define SK_ICALL_VERIFY_PARAMETER_V(_param, _value) if (!(_value)) { SK_CONSOLE_ERROR("{} called with with invalid value for parameter '{}'", SK_FUNCTION_NAME, #_param); }

	static Entity GetEntity(uint64_t entityID)
	{
		auto currentScene = ScriptEngine::Get().GetCurrentSceen();
		SK_CORE_VERIFY(currentScene, "No active Scene");
		return currentScene->TryGetEntityByUUID(entityID);
	}

	void ScriptGlue::Initialize(Coral::ManagedAssembly& assembly)
	{
		RegisterComponents(assembly);
		RegisterInternalCalls(assembly);
	}

	void ScriptGlue::Shutdown()
	{
		s_HasComponentFunctions.clear();
		s_AddComponentFunctions.clear();
		s_RemoveComponentFunctions.clear();
	}


	void ScriptGlue::RegisterComponents(Coral::ManagedAssembly& assembly)
	{
#define SK_REGISTER_COMPONENT(_component)\
		s_HasComponentFunctions[assembly.GetType("Shark." #_component).GetTypeId()] = [](Entity entity) -> bool { return entity.HasComponent<_component>(); };\
		s_AddComponentFunctions[assembly.GetType("Shark." #_component).GetTypeId()] = [](Entity entity) -> void { entity.AddComponent<_component>(); };\
		s_RemoveComponentFunctions[assembly.GetType("Shark." #_component).GetTypeId()] = [](Entity entity) -> void { entity.RemoveComponent<_component>(); }

		SK_REGISTER_COMPONENT(TagComponent);
		SK_REGISTER_COMPONENT(TransformComponent);
		SK_REGISTER_COMPONENT(CameraComponent);
		SK_REGISTER_COMPONENT(SpriteRendererComponent);
		SK_REGISTER_COMPONENT(CircleRendererComponent);
		SK_REGISTER_COMPONENT(RigidBody2DComponent);
		SK_REGISTER_COMPONENT(BoxCollider2DComponent);
		SK_REGISTER_COMPONENT(CircleCollider2DComponent);
#undef SK_REGISTER_COMPONENT
	}

	void ScriptGlue::RegisterInternalCalls(Coral::ManagedAssembly& assembly)
	{
		#define ADD_ICALL(_func) assembly.AddInternalCall("Shark.InternalCalls", #_func, &InternalCalls::_func)
		ADD_ICALL(AssetHandle_IsValid);

		ADD_ICALL(Application_GetWidth);
		ADD_ICALL(Application_GetHeight);

		ADD_ICALL(Log_LogMessage);

		ADD_ICALL(Input_IsKeyStateSet);
		ADD_ICALL(Input_IsMouseStateSet);
		ADD_ICALL(Input_GetMouseScroll);
		ADD_ICALL(Input_GetMousePos);

		ADD_ICALL(Matrix4_Inverse);
		ADD_ICALL(Matrix4_Matrix4MulMatrix4);
		ADD_ICALL(Matrix4_Matrix4MulVector4);

		ADD_ICALL(Scene_IsEntityValid);
		ADD_ICALL(Scene_CreateEntity);
		ADD_ICALL(Scene_InstantiatePrefab);
		ADD_ICALL(Scene_InstantiateChildPrefab);
		ADD_ICALL(Scene_DestroyEntity);
		ADD_ICALL(Scene_FindEntityByTag);

		ADD_ICALL(Entity_GetParent);
		ADD_ICALL(Entity_SetParent);
		ADD_ICALL(Entity_GetChildren);
		ADD_ICALL(Entity_HasComponent);
		ADD_ICALL(Entity_AddComponent);
		ADD_ICALL(Entity_RemoveComponent);

		ADD_ICALL(TagComponent_GetTag);
		ADD_ICALL(TagComponent_SetTag);

		ADD_ICALL(TransformComponent_GetTranslation);
		ADD_ICALL(TransformComponent_SetTranslation);
		ADD_ICALL(TransformComponent_GetRotation);
		ADD_ICALL(TransformComponent_SetRotation);
		ADD_ICALL(TransformComponent_GetScale);
		ADD_ICALL(TransformComponent_SetScale);
		ADD_ICALL(TransformComponent_GetLocalTransform);
		ADD_ICALL(TransformComponent_SetLocalTransform);
		ADD_ICALL(TransformComponent_GetWorldTransform);
		ADD_ICALL(TransformComponent_SetWorldTransform);

		ADD_ICALL(SpriteRendererComponent_GetColor);
		ADD_ICALL(SpriteRendererComponent_SetColor);
		ADD_ICALL(SpriteRendererComponent_SetTextureHandle);
		ADD_ICALL(SpriteRendererComponent_GetTextureHandle);
		ADD_ICALL(SpriteRendererComponent_GetTilingFactor);
		ADD_ICALL(SpriteRendererComponent_SetTilingFactor);

		ADD_ICALL(CircleRendererComponent_GetColor);
		ADD_ICALL(CircleRendererComponent_SetColor);
		ADD_ICALL(CircleRendererComponent_GetThickness);
		ADD_ICALL(CircleRendererComponent_SetThickness);
		ADD_ICALL(CircleRendererComponent_GetFade);
		ADD_ICALL(CircleRendererComponent_SetFade);

		ADD_ICALL(CameraComponent_GetProjection);
		ADD_ICALL(CameraComponent_SetProjection);
		ADD_ICALL(CameraComponent_SetIsPerspective);
		ADD_ICALL(CameraComponent_GetIsPerspective);
		ADD_ICALL(CameraComponent_GetAspectratio);
		ADD_ICALL(CameraComponent_SetAspectratio);
		ADD_ICALL(CameraComponent_GetPerspectiveFOV);
		ADD_ICALL(CameraComponent_SetPerspectiveFOV);
		ADD_ICALL(CameraComponent_GetOrthographicZoom);
		ADD_ICALL(CameraComponent_SetOrthographicZoom);
		ADD_ICALL(CameraComponent_GetPerspectiveNear);
		ADD_ICALL(CameraComponent_SetPerspectiveNear);
		ADD_ICALL(CameraComponent_GetPerspectiveFar);
		ADD_ICALL(CameraComponent_SetPerspectiveFar);
		ADD_ICALL(CameraComponent_GetOrthographicNear);
		ADD_ICALL(CameraComponent_SetOrthographicNear);
		ADD_ICALL(CameraComponent_GetOrthographicFar);
		ADD_ICALL(CameraComponent_SetOrthographicFar);

		ADD_ICALL(Physics2D_GetGravity);
		ADD_ICALL(Physics2D_SetGravity);
		ADD_ICALL(Physics2D_GetAllowSleep);
		ADD_ICALL(Physics2D_SetAllowSleep);

		ADD_ICALL(RigidBody2DComponent_GetBodyType);
		ADD_ICALL(RigidBody2DComponent_SetBodyType);
		ADD_ICALL(RigidBody2DComponent_GetPosition);
		ADD_ICALL(RigidBody2DComponent_SetPosition);
		ADD_ICALL(RigidBody2DComponent_GetRotation);
		ADD_ICALL(RigidBody2DComponent_SetRotation);
		ADD_ICALL(RigidBody2DComponent_GetLocalCenter);
		ADD_ICALL(RigidBody2DComponent_GetWorldCenter);
		ADD_ICALL(RigidBody2DComponent_GetLinearVelocity);
		ADD_ICALL(RigidBody2DComponent_SetLinearVelocity);
		ADD_ICALL(RigidBody2DComponent_GetAngularVelocity);
		ADD_ICALL(RigidBody2DComponent_SetAngularVelocity);
		ADD_ICALL(RigidBody2DComponent_ApplyForce);
		ADD_ICALL(RigidBody2DComponent_ApplyForceToCenter);
		ADD_ICALL(RigidBody2DComponent_ApplyTorque);
		ADD_ICALL(RigidBody2DComponent_GetGravityScale);
		ADD_ICALL(RigidBody2DComponent_SetGravityScale);
		ADD_ICALL(RigidBody2DComponent_GetLinearDamping);
		ADD_ICALL(RigidBody2DComponent_SetLinearDamping);
		ADD_ICALL(RigidBody2DComponent_GetAngularDamping);
		ADD_ICALL(RigidBody2DComponent_SetAngularDamping);
		ADD_ICALL(RigidBody2DComponent_IsBullet);
		ADD_ICALL(RigidBody2DComponent_SetBullet);
		ADD_ICALL(RigidBody2DComponent_IsSleepingAllowed);
		ADD_ICALL(RigidBody2DComponent_SetSleepingAllowed);
		ADD_ICALL(RigidBody2DComponent_IsAwake);
		ADD_ICALL(RigidBody2DComponent_SetAwake);
		ADD_ICALL(RigidBody2DComponent_IsEnabled);
		ADD_ICALL(RigidBody2DComponent_SetEnabled);
		ADD_ICALL(RigidBody2DComponent_IsFixedRotation);
		ADD_ICALL(RigidBody2DComponent_SetFixedRotation);

		ADD_ICALL(BoxCollider2DComponent_SetSensor);
		ADD_ICALL(BoxCollider2DComponent_IsSensor);
		ADD_ICALL(BoxCollider2DComponent_SetDensity);
		ADD_ICALL(BoxCollider2DComponent_GetDensity);
		ADD_ICALL(BoxCollider2DComponent_SetFriction);
		ADD_ICALL(BoxCollider2DComponent_GetFriction);
		ADD_ICALL(BoxCollider2DComponent_SetRestitution);
		ADD_ICALL(BoxCollider2DComponent_GetRestitution);
		ADD_ICALL(BoxCollider2DComponent_SetRestitutionThreshold);
		ADD_ICALL(BoxCollider2DComponent_GetRestitutionThreshold);
		ADD_ICALL(BoxCollider2DComponent_GetSize);
		ADD_ICALL(BoxCollider2DComponent_SetSize);
		ADD_ICALL(BoxCollider2DComponent_GetOffset);
		ADD_ICALL(BoxCollider2DComponent_SetOffset);
		ADD_ICALL(BoxCollider2DComponent_GetRotation);
		ADD_ICALL(BoxCollider2DComponent_SetRotation);

		ADD_ICALL(CircleCollider2DComponent_SetSensor);
		ADD_ICALL(CircleCollider2DComponent_IsSensor);
		ADD_ICALL(CircleCollider2DComponent_SetDensity);
		ADD_ICALL(CircleCollider2DComponent_GetDensity);
		ADD_ICALL(CircleCollider2DComponent_SetFriction);
		ADD_ICALL(CircleCollider2DComponent_GetFriction);
		ADD_ICALL(CircleCollider2DComponent_SetRestitution);
		ADD_ICALL(CircleCollider2DComponent_GetRestitution);
		ADD_ICALL(CircleCollider2DComponent_SetRestitutionThreshold);
		ADD_ICALL(CircleCollider2DComponent_GetRestitutionThreshold);
		ADD_ICALL(CircleCollider2DComponent_GetRadius);
		ADD_ICALL(CircleCollider2DComponent_SetRadius);
		ADD_ICALL(CircleCollider2DComponent_GetOffset);
		ADD_ICALL(CircleCollider2DComponent_SetOffset);
		ADD_ICALL(CircleCollider2DComponent_GetRotation);
		ADD_ICALL(CircleCollider2DComponent_SetRotation);
		#undef ADD_ICALL

		assembly.UploadInternalCalls();
	}

	namespace InternalCalls
	{

		#pragma region AssetHandle

		Coral::Bool32 AssetHandle_IsValid(AssetHandle assetHandle)
		{
			return AssetManager::IsValidAssetHandle(assetHandle);
		}

		#pragma endregion

		#pragma region Application

		uint32_t Application_GetWidth()
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			return currentScene->GetViewportWidth();
		}

		uint32_t Application_GetHeight()
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			return currentScene->GetViewportHeight();
		}

		#pragma endregion

		#pragma region Log

		void Log_LogMessage(LogLevel level, Coral::String message)
		{
			std::string msg = message;
			switch (level)
			{
				case LogLevel::Trace:
					SK_CONSOLE_TRACE(msg);
					break;
				case LogLevel::Info:
					SK_CONSOLE_INFO(msg);
					break;
				case LogLevel::Warn:
					SK_CONSOLE_WARN(msg);
					break;
				case LogLevel::Error:
					SK_CONSOLE_ERROR(msg);
					break;
				case LogLevel::Critical:
					SK_CONSOLE_CRITICAL(msg);
					break;
				case LogLevel::Debug:
					SK_CONSOLE_DEBUG(msg);
					break;
			}
		}

		#pragma endregion

		#pragma region Input

		Coral::Bool32 Input_IsKeyStateSet(KeyCode key, KeyState keyState, Coral::Bool32 allowRepeate)
		{
			auto& app = Application::Get();
			if (app.GetSpecification().EnableImGui)
			{
				const ImGuiLayer& imguiLayer = app.GetImGuiLayer();
				if (imguiLayer.BlocksKeyboardEvents())
					return false;
			}

			if (allowRepeate && keyState == KeyState::Pressed)
				return Input::IsKeyPressed(key, allowRepeate);

			return Input::GetKeyState(key) == keyState;
		}

		Coral::Bool32 Input_IsMouseStateSet(MouseButton button, MouseState mouseState)
		{
			auto& app = Application::Get();
			if (app.GetSpecification().EnableImGui)
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
			if (app.GetSpecification().EnableImGui)
			{
				const ImGuiLayer& imguiLayer = app.GetImGuiLayer();
				if (imguiLayer.BlocksMouseEvents())
					return false;
			}

			return Input::GetYScroll();
		}

		void Input_GetMousePos(glm::ivec2* out_MousePos)
		{
			auto mousePos = Input::GetMousePosition();

			const auto& app = Application::Get();
			if (app.GetSpecification().EnableImGui)
			{
				const ImGuiWindow* viewportWindow = ImGui::FindWindowByID(app.GetImGuiLayer().GetMainViewportID());
				if (viewportWindow)
					mousePos -= app.GetWindow().ScreenToWindow({ viewportWindow->Pos.x, viewportWindow->Pos.y });
			}

			out_MousePos->x = (int)mousePos.x;
			out_MousePos->y = (int)mousePos.y;
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

		#pragma region Scene

		Coral::Bool32 Scene_IsEntityValid(uint64_t entityID)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			return currentScene->IsValidEntityID(entityID);
		}

		uint64_t Scene_CreateEntity(Coral::String name)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			Entity newEntity = currentScene->CreateEntity(name);
			return newEntity.GetUUID();
		}

		uint64_t Scene_InstantiatePrefab(AssetHandle handle, glm::vec3* translation, glm::vec3* rotation, glm::vec3* scale)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			SK_ICALL_VERIFY_PARAMETER(AssetManager::IsValidAssetHandle(handle));

			Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(handle);
			Entity entity = currentScene->Instansitate(prefab, translation, rotation, scale);
			return entity.GetUUID();
		}

		uint64_t Scene_InstantiateChildPrefab(AssetHandle handle, uint64_t parentID, glm::vec3* translation, glm::vec3* rotation, glm::vec3* scale)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			Entity parent = currentScene->TryGetEntityByUUID(parentID);
			SK_ICALL_VERIFY_PARAMETER(parent);
			SK_ICALL_VERIFY_PARAMETER_V(handle, AssetManager::IsValidAssetHandle(handle));

			Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(handle);
			if (prefab)
			{
				Entity entity = currentScene->InstansitateChild(prefab, parent, translation, rotation, scale);
				return entity.GetUUID();
			}
			return UUID::Invalid;
		}

		void Scene_DestroyEntity(uint64_t entityID)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			Entity entity = currentScene->TryGetEntityByUUID(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);

			currentScene->DestroyEntity(entity);
		}

		uint64_t Scene_FindEntityByTag(Coral::String Tag)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			Entity entity = currentScene->FindEntityByTag(Tag);
			return entity ? (uint64_t)entity.GetUUID() : UUID::Invalid;
		}

		#pragma endregion

		#pragma region Entity

		uint64_t Entity_GetParent(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);

			if (entity && entity.HasParent())
				return entity.ParentID();
			return UUID::Invalid;
		}

		void Entity_SetParent(uint64_t entityID, uint64_t parentID)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
			{
				// #TODO(moro): error
				return;
			}

			Entity entity = currentScene->GetEntityByID(entityID);
			if (!parentID)
			{
				currentScene->UnparentEntity(entity);
				return;
			}

			Entity parent = currentScene->GetEntityByID(parentID);
			currentScene->ParentEntity(entity, parent);
		}

		Coral::Array<uint64_t> Entity_GetChildren(uint64_t entityID)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return {};

			Entity entity = currentScene->GetEntityByID(entityID);
			const auto& children = entity.Children();
			auto childrenIDs = Coral::Array<uint64_t>::New(children.size());

			for (uint32_t i = 0; i < children.size(); i++)
				childrenIDs[i] = children[i];

			return childrenIDs;
		}

		Coral::Bool32 Entity_HasComponent(uint64_t entityID, Coral::ReflectionType reflectionType)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return false;

			Coral::Type& type = reflectionType;
			Entity entity = currentScene->GetEntityByID(entityID);
			
			auto hasCompFunc = s_HasComponentFunctions.at(type.GetTypeId());
			return hasCompFunc(entity);
		}

		void Entity_AddComponent(uint64_t entityID, Coral::ReflectionType reflectionType)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return;

			Coral::Type& type = reflectionType;
			Entity entity = currentScene->GetEntityByID(entityID);

			auto addCompFunc = s_AddComponentFunctions.at(type.GetTypeId());
			addCompFunc(entity);
		}

		void Entity_RemoveComponent(uint64_t entityID, Coral::ReflectionType reflectionType)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return;

			Coral::Type& type = reflectionType;
			Entity entity = currentScene->GetEntityByID(entityID);

			auto removeCompFunc = s_RemoveComponentFunctions.at(type.GetTypeId());
			removeCompFunc(entity);
		}

		#pragma endregion

		#pragma region TagComponent

		Coral::String TagComponent_GetTag(uint64_t entityID)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return {};

			Entity entity = currentScene->GetEntityByID(entityID);
			return Coral::String::New(entity.Tag());
		}

		void TagComponent_SetTag(uint64_t entityID, Coral::String tag)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return;

			Entity entity = currentScene->GetEntityByID(entityID);
			entity.Tag() = tag;
		}

		#pragma endregion

		#pragma region TransformComponent

		void TransformComponent_GetTranslation(uint64_t entityID, glm::vec3* out_Translation)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return;

			Entity entity = currentScene->GetEntityByID(entityID);
			auto& transform = entity.Transform();
			*out_Translation = transform.Translation;
		}

		void TransformComponent_SetTranslation(uint64_t entityID, glm::vec3* translation)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return;

			Entity entity = currentScene->GetEntityByID(entityID);
			auto& transform = entity.Transform();
			transform.Translation = *translation;

			if (entity.HasComponent<RigidBody2DComponent>())
			{
				auto& rigidBody = entity.GetComponent<RigidBody2DComponent>();
				if (rigidBody.RuntimeBody)
					rigidBody.RuntimeBody->SetTransform({ translation->x, translation->y }, rigidBody.RuntimeBody->GetAngle());
			}
		}

		void TransformComponent_GetRotation(uint64_t entityID, glm::vec3* out_Rotation)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return;

			Entity entity = currentScene->GetEntityByID(entityID);
			auto& transform = entity.Transform();
			*out_Rotation = transform.Rotation;
		}

		void TransformComponent_SetRotation(uint64_t entityID, glm::vec3* rotation)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return;

			Entity entity = currentScene->GetEntityByID(entityID);
			auto& transform = entity.Transform();
			transform.Rotation = *rotation;

			if (entity.HasComponent<RigidBody2DComponent>())
			{
				auto& rigidBody = entity.GetComponent<RigidBody2DComponent>();
				if (rigidBody.RuntimeBody)
					rigidBody.RuntimeBody->SetTransform(rigidBody.RuntimeBody->GetPosition(), rotation->z);
			}
		}

		void TransformComponent_GetScale(uint64_t entityID, glm::vec3* out_Scaling)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return;

			Entity entity = currentScene->GetEntityByID(entityID);
			auto& transform = entity.Transform();
			*out_Scaling = transform.Scale;
		}

		void TransformComponent_SetScale(uint64_t entityID, glm::vec3* scaling)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return;

			Entity entity = currentScene->GetEntityByID(entityID);
			auto& transform = entity.Transform();
			transform.Scale = *scaling;
		}

		void TransformComponent_GetLocalTransform(uint64_t entityID, TransformComponent* out_LocalTransform)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return;

			Entity entity = currentScene->GetEntityByID(entityID);
			*out_LocalTransform = entity.Transform();
		}

		void TransformComponent_SetLocalTransform(uint64_t entityID, TransformComponent* localTransform)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return;

			Entity entity = currentScene->GetEntityByID(entityID);
			entity.Transform() = *localTransform;

			if (entity.HasComponent<RigidBody2DComponent>())
			{
				auto& rigidBody = entity.GetComponent<RigidBody2DComponent>();
				if (rigidBody.RuntimeBody)
				{
					const auto& transform = entity.Transform();
					rigidBody.RuntimeBody->SetTransform({ transform.Translation.x, transform.Translation.y }, transform.Rotation.z);
				}
			}
		}

		void TransformComponent_GetWorldTransform(uint64_t entityID, TransformComponent* out_WorldTransform)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return;

			Entity entity = currentScene->GetEntityByID(entityID);
			if (!entity.HasParent())
			{
				*out_WorldTransform = entity.Transform();
				return;
			}

			glm::mat4 worldTransform = currentScene->GetWorldSpaceTransformMatrix(entity);
			Math::DecomposeTransform(worldTransform, out_WorldTransform->Translation, out_WorldTransform->Rotation, out_WorldTransform->Scale);
		}

		void TransformComponent_SetWorldTransform(uint64_t entityID, TransformComponent* worldTransform)
		{
			auto currentScene = ScriptEngine::Get().GetCurrentSceen();
			if (!currentScene->IsValidEntityID(entityID))
				return;

			Entity entity = currentScene->GetEntityByID(entityID);
			if (!entity.HasParent())
			{
				entity.Transform() = *worldTransform;
				return;
			}

			entity.Transform() = *worldTransform;
			currentScene->ConvertToLocaSpace(entity);

			if (entity.HasComponent<RigidBody2DComponent>())
			{
				auto& rigidBody = entity.GetComponent<RigidBody2DComponent>();
				if (rigidBody.RuntimeBody)
				{
					const auto& transform = entity.Transform();
					rigidBody.RuntimeBody->SetTransform({ transform.Translation.x, transform.Translation.y }, transform.Rotation.z);
				}
			}
		}

		#pragma endregion

		#pragma region SpriteRendererComponent

		void SpriteRendererComponent_GetColor(uint64_t entityID, glm::vec4* out_Color)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<SpriteRendererComponent>());
			*out_Color = entity.GetComponent<SpriteRendererComponent>().Color;
		}

		void SpriteRendererComponent_SetColor(uint64_t entityID, glm::vec4* color)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<SpriteRendererComponent>());
			entity.GetComponent<SpriteRendererComponent>().Color = *color;
		}

		AssetHandle SpriteRendererComponent_GetTextureHandle(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<SpriteRendererComponent>());
			return entity.GetComponent<SpriteRendererComponent>().TextureHandle;
		}

		void SpriteRendererComponent_SetTextureHandle(uint64_t entityID, AssetHandle textureHandle)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<SpriteRendererComponent>());
			entity.GetComponent<SpriteRendererComponent>().TextureHandle = textureHandle;
		}

		void SpriteRendererComponent_GetTilingFactor(uint64_t entityID, glm::vec2* outTilingFactor)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<SpriteRendererComponent>());
			*outTilingFactor = entity.GetComponent<SpriteRendererComponent>().TilingFactor;
		}

		void SpriteRendererComponent_SetTilingFactor(uint64_t entityID, glm::vec2* tilingFactor)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<SpriteRendererComponent>());
			entity.GetComponent<SpriteRendererComponent>().TilingFactor = *tilingFactor;
		}

		#pragma endregion

		#pragma region CricleRendererComponent

		void CircleRendererComponent_GetColor(uint64_t entityID, glm::vec4* out_Color)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleRendererComponent>());
			*out_Color = entity.GetComponent<CircleRendererComponent>().Color;
		}

		void CircleRendererComponent_SetColor(uint64_t entityID, glm::vec4* color)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleRendererComponent>());
			entity.GetComponent<CircleRendererComponent>().Color = *color;
		}

		float CircleRendererComponent_GetThickness(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleRendererComponent>());
			return entity.GetComponent<CircleRendererComponent>().Thickness;
		}

		void CircleRendererComponent_SetThickness(uint64_t entityID, float thickness)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleRendererComponent>());
			entity.GetComponent<CircleRendererComponent>().Thickness = thickness;
		}

		float CircleRendererComponent_GetFade(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleRendererComponent>());
			return entity.GetComponent<CircleRendererComponent>().Fade;
		}

		void CircleRendererComponent_SetFade(uint64_t entityID, float fade)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleRendererComponent>());
			entity.GetComponent<CircleRendererComponent>().Fade = fade;
		}

		#pragma endregion

		#pragma region CameraComponent

		void CameraComponent_RecalculateProjectionMatrix(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			entity.GetComponent<CameraComponent>().Recalculate();
		}

		void CameraComponent_GetProjection(uint64_t entityID, glm::mat4* out_Projection)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			*out_Projection = entity.GetComponent<CameraComponent>().GetProjection();
		}

		void CameraComponent_SetProjection(uint64_t entityID, glm::mat4* projection)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			entity.GetComponent<CameraComponent>().Camera.SetProjection(*projection);
		}

		Coral::Bool32 CameraComponent_GetIsPerspective(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			return entity.GetComponent<CameraComponent>().IsPerspective;
		}

		void CameraComponent_SetIsPerspective(uint64_t entityID, bool isPerspective)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			entity.GetComponent<CameraComponent>().IsPerspective = isPerspective;
		}

		void CameraComponent_SetPerspective(uint64_t entityID, float aspectratio, float perspectiveFOV, float clipnear, float clipfar)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());

			auto& component = entity.GetComponent<CameraComponent>();
			component.IsPerspective = true;
			component.AspectRatio = aspectratio;
			component.PerspectiveFOV = perspectiveFOV;
			component.Near = clipnear;
			component.Far = clipfar;
		}

		void CameraComponent_SetOrthographic(uint64_t entityID, float aspectratio, float orthographicSize, float clipnear, float clipfar)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());

			auto& component = entity.GetComponent<CameraComponent>();
			component.IsPerspective = false;
			component.AspectRatio = aspectratio;
			component.OrthographicSize = orthographicSize;
			component.Near = clipnear;
			component.Far = clipfar;
		}

		float CameraComponent_GetAspectratio(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			return entity.GetComponent<CameraComponent>().AspectRatio;
		}

		void CameraComponent_SetAspectratio(uint64_t entityID, float aspectratio)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			entity.GetComponent<CameraComponent>().AspectRatio = aspectratio;
		}

		float CameraComponent_GetPerspectiveFOV(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			return glm::degrees(entity.GetComponent<CameraComponent>().PerspectiveFOV);
		}

		void CameraComponent_SetPerspectiveFOV(uint64_t entityID, float fov)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			entity.GetComponent<CameraComponent>().PerspectiveFOV = glm::radians(fov);
		}

		float CameraComponent_GetOrthographicZoom(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			return entity.GetComponent<CameraComponent>().OrthographicSize;
		}

		void CameraComponent_SetOrthographicZoom(uint64_t entityID, float size)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			entity.GetComponent<CameraComponent>().OrthographicSize = size;
		}

		float CameraComponent_GetPerspectiveNear(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			return entity.GetComponent<CameraComponent>().Near;
		}

		void CameraComponent_SetPerspectiveNear(uint64_t entityID, float clipnear)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			entity.GetComponent<CameraComponent>().Near = clipnear;
		}

		float CameraComponent_GetPerspectiveFar(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			return entity.GetComponent<CameraComponent>().Far;
		}

		void CameraComponent_SetPerspectiveFar(uint64_t entityID, float clipfar)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			entity.GetComponent<CameraComponent>().Far = clipfar;
		}

		float CameraComponent_GetOrthographicNear(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			return entity.GetComponent<CameraComponent>().Near;
		}

		void CameraComponent_SetOrthographicNear(uint64_t entityID, float clipnear)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			entity.GetComponent<CameraComponent>().Near = clipnear;
		}

		float CameraComponent_GetOrthographicFar(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			return entity.GetComponent<CameraComponent>().Far;
		}

		void CameraComponent_SetOrthographicFar(uint64_t entityID, float clipfar)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CameraComponent>());
			entity.GetComponent<CameraComponent>().Far = clipfar;
		}

		#pragma endregion

		#pragma region Physics2D

		void Physics2D_GetGravity(glm::vec2* out_Gravity)
		{
			Ref<Scene> currentScene = ScriptEngine::Get().GetCurrentSceen();
			auto& physicsScene = currentScene->GetPhysicsScene();
			SK_ICALL_VERIFY_PARAMETER(physicsScene.Active());


			auto gravity = physicsScene.GetWorld()->GetGravity();
			out_Gravity->x = gravity.x;
			out_Gravity->y = gravity.y;
		}

		void Physics2D_SetGravity(glm::vec2* gravity)
		{
			Ref<Scene> currentScene = ScriptEngine::Get().GetCurrentSceen();
			auto& physicsScene = currentScene->GetPhysicsScene();
			SK_ICALL_VERIFY_PARAMETER(physicsScene.Active());

			physicsScene.GetWorld()->SetGravity({ gravity->x, gravity->y });
		}

		Coral::Bool32 Physics2D_GetAllowSleep()
		{
			Ref<Scene> currentScene = ScriptEngine::Get().GetCurrentSceen();
			auto& physicsScene = currentScene->GetPhysicsScene();
			SK_ICALL_VERIFY_PARAMETER(physicsScene.Active());

			return physicsScene.GetWorld()->GetAllowSleeping();
		}

		void Physics2D_SetAllowSleep(bool allowSleep)
		{
			Ref<Scene> currentScene = ScriptEngine::Get().GetCurrentSceen();
			auto& physicsScene = currentScene->GetPhysicsScene();
			SK_ICALL_VERIFY_PARAMETER(physicsScene.Active());

			physicsScene.GetWorld()->SetAllowSleeping(allowSleep);
		}

		#pragma endregion

		#pragma region RigidBody2DComponent

		RigidbodyType RigidBody2DComponent_GetBodyType(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());
			return entity.GetComponent<RigidBody2DComponent>().Type;
		}

		void RigidBody2DComponent_SetBodyType(uint64_t entityID, RigidbodyType bodyType)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			if (component.Type == bodyType)
				return;

			component.Type = bodyType;

			SK_CORE_VERIFY(component.RuntimeBody);
			component.RuntimeBody->SetType(Physics2DUtils::ConvertBodyType(component.Type));
		}

		void RigidBody2DComponent_GetPosition(uint64_t entityID, glm::vec2* outPosition)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			const auto& component = entity.GetComponent<RigidBody2DComponent>();

			SK_CORE_VERIFY(component.RuntimeBody);
			const auto& bodyPosition = component.RuntimeBody->GetPosition();
			*outPosition = { bodyPosition.x, bodyPosition.y };
		}

		void RigidBody2DComponent_SetPosition(uint64_t entityID, glm::vec2* position)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			body->SetTransform({ position->x, position->y }, body->GetAngle());
		}

		float RigidBody2DComponent_GetRotation(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			const auto& component = entity.GetComponent<RigidBody2DComponent>();

			SK_CORE_VERIFY(component.RuntimeBody);
			return component.RuntimeBody->GetAngle();
		}

		void RigidBody2DComponent_SetRotation(uint64_t entityID, float rotation)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			body->SetTransform(body->GetPosition(), rotation);
		}

		void RigidBody2DComponent_GetLocalCenter(uint64_t entityID, glm::vec2* out_LocalCenter)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			const b2Vec2 lc = body->GetLocalCenter();
			*out_LocalCenter = { lc.x, lc.y };
		}

		void RigidBody2DComponent_GetWorldCenter(uint64_t entityID, glm::vec2* out_WorldCenter)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			const b2Vec2 wc = body->GetWorldCenter();
			*out_WorldCenter = { wc.x, wc.y };
		}

		void RigidBody2DComponent_GetLinearVelocity(uint64_t entityID, glm::vec2* out_LinearVelocity)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			const b2Vec2& lv = body->GetLinearVelocity();
			*out_LinearVelocity = { lv.x, lv.y };
		}

		void RigidBody2DComponent_SetLinearVelocity(uint64_t entityID, glm::vec2* linearVelocity)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			body->SetLinearVelocity({ linearVelocity->x, linearVelocity->y });
		}

		float RigidBody2DComponent_GetAngularVelocity(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			return body->GetAngularVelocity();
		}

		void RigidBody2DComponent_SetAngularVelocity(uint64_t entityID, float angularVelocity)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			body->SetAngularVelocity(angularVelocity);
		}

		float RigidBody2DComponent_GetGravityScale(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			return body->GetGravityScale();
		}

		void RigidBody2DComponent_SetGravityScale(uint64_t entityID, float gravityScale)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			component.GravityScale = gravityScale;

			SK_CORE_VERIFY(component.RuntimeBody);
			component.RuntimeBody->SetGravityScale(component.GravityScale);
		}

		float RigidBody2DComponent_GetLinearDamping(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			return body->GetLinearDamping();
		}

		void RigidBody2DComponent_SetLinearDamping(uint64_t entityID, float linearDamping)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			body->SetLinearDamping(linearDamping);
		}

		float RigidBody2DComponent_GetAngularDamping(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			return body->GetAngularDamping();
		}

		void RigidBody2DComponent_SetAngularDamping(uint64_t entityID, float angularDamping)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			body->SetAngularDamping(angularDamping);
		}

		Coral::Bool32 RigidBody2DComponent_IsBullet(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			return body->IsBullet();
		}

		void RigidBody2DComponent_SetBullet(uint64_t entityID, bool bullet)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			component.IsBullet = bullet;

			SK_CORE_VERIFY(component.RuntimeBody);
			component.RuntimeBody->SetBullet(component.IsBullet);
		}

		Coral::Bool32 RigidBody2DComponent_IsSleepingAllowed(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			return body->IsSleepingAllowed();
		}

		void RigidBody2DComponent_SetSleepingAllowed(uint64_t entityID, bool sleepingAllowed)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			component.AllowSleep = sleepingAllowed;

			SK_CORE_VERIFY(component.RuntimeBody);
			component.RuntimeBody->SetSleepingAllowed(component.AllowSleep);
		}

		Coral::Bool32 RigidBody2DComponent_IsAwake(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			return body->IsAwake();
		}

		void RigidBody2DComponent_SetAwake(uint64_t entityID, bool awake)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			component.Awake = awake;

			SK_CORE_VERIFY(component.RuntimeBody);
			component.RuntimeBody->SetAwake(component.Awake);
		}

		Coral::Bool32 RigidBody2DComponent_IsEnabled(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			return body->IsEnabled();
		}

		void RigidBody2DComponent_SetEnabled(uint64_t entityID, bool enabled)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			component.Enabled = enabled;

			SK_CORE_VERIFY(component.RuntimeBody);
			component.RuntimeBody->SetEnabled(component.Enabled);
		}

		Coral::Bool32 RigidBody2DComponent_IsFixedRotation(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			return body->IsFixedRotation();
		}

		void RigidBody2DComponent_SetFixedRotation(uint64_t entityID, bool fixedRotation)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			auto& component = entity.GetComponent<RigidBody2DComponent>();
			component.FixedRotation = fixedRotation;

			SK_CORE_VERIFY(component.RuntimeBody);
			component.RuntimeBody->SetFixedRotation(component.FixedRotation);
		}
		
		void RigidBody2DComponent_ApplyForce(uint64_t entityID, glm::vec2* force, glm::vec2* point, PhysicsForce2DType forceType)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			if (forceType == PhysicsForce2DType::Force)
				body->ApplyForce({ force->x, force->y }, { point->x, point->y }, true);
			else
				body->ApplyLinearImpulse({ force->x, force->y }, { point->x, point->y }, true);
		}

		void RigidBody2DComponent_ApplyForceToCenter(uint64_t entityID, glm::vec2* force, PhysicsForce2DType forceType)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			if (forceType == PhysicsForce2DType::Force)
				body->ApplyForceToCenter({ force->x, force->y }, true);
			else
				body->ApplyLinearImpulseToCenter({ force->x, force->y }, true);
		}

		void RigidBody2DComponent_ApplyTorque(uint64_t entityID, float torque, PhysicsForce2DType forceType)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<RigidBody2DComponent>());

			b2Body* body = entity.GetComponent<RigidBody2DComponent>().RuntimeBody;
			SK_CORE_VERIFY(body);

			if (forceType == PhysicsForce2DType::Force)
				body->ApplyTorque(torque, true);
			else
				body->ApplyAngularImpulse(torque, true);
		}

		#pragma endregion

		#pragma region BoxCollider2DComponent

		Coral::Bool32 BoxCollider2DComponent_IsSensor(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();

			SK_CORE_VERIFY(component.RuntimeCollider);
			return component.RuntimeCollider->IsSensor();
		}

		void BoxCollider2DComponent_SetSensor(uint64_t entityID, bool sensor)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();
			component.IsSensor = sensor;

			SK_CORE_VERIFY(component.RuntimeCollider);
			component.RuntimeCollider->SetSensor(sensor);
		}

		float BoxCollider2DComponent_GetDensity(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();

			SK_CORE_VERIFY(component.RuntimeCollider);
			return component.RuntimeCollider->GetDensity();
		}

		void BoxCollider2DComponent_SetDensity(uint64_t entityID, float density)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();
			component.Density = density;

			SK_CORE_VERIFY(component.RuntimeCollider);
			component.RuntimeCollider->SetDensity(density);
		}

		float BoxCollider2DComponent_GetFriction(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();

			SK_CORE_VERIFY(component.RuntimeCollider);
			return component.RuntimeCollider->GetFriction();
		}

		void BoxCollider2DComponent_SetFriction(uint64_t entityID, float friction)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();
			component.Friction = friction;

			SK_CORE_VERIFY(component.RuntimeCollider);
			component.RuntimeCollider->SetFriction(friction);
		}

		float BoxCollider2DComponent_GetRestitution(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();

			SK_CORE_VERIFY(component.RuntimeCollider);
			return component.RuntimeCollider->GetRestitution();
		}

		void BoxCollider2DComponent_SetRestitution(uint64_t entityID, float restitution)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();
			component.Restitution = restitution;

			SK_CORE_VERIFY(component.RuntimeCollider);
			component.RuntimeCollider->SetRestitution(restitution);
		}

		float BoxCollider2DComponent_GetRestitutionThreshold(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();

			SK_CORE_VERIFY(component.RuntimeCollider);
			return component.RuntimeCollider->GetRestitutionThreshold();
		}

		void BoxCollider2DComponent_SetRestitutionThreshold(uint64_t entityID, float restitutionThreshold)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();
			component.RestitutionThreshold = restitutionThreshold;

			SK_CORE_VERIFY(component.RuntimeCollider);
			component.RuntimeCollider->SetRestitutionThreshold(restitutionThreshold);
		}

		void BoxCollider2DComponent_GetSize(uint64_t entityID, glm::vec2* out_Size)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();
			*out_Size = component.Size;
		}

		void BoxCollider2DComponent_SetSize(uint64_t entityID, glm::vec2* size)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();
			const auto& transform = entity.Transform();
			component.Size = *size;

			SK_CORE_VERIFY(component.RuntimeCollider);
			SK_CORE_VERIFY(component.RuntimeCollider->GetType() == b2Shape::e_polygon);
			b2PolygonShape* shape = (b2PolygonShape*)component.RuntimeCollider->GetShape();
			shape->SetAsBox(component.Size.x * transform.Scale.x, component.Size.y * transform.Scale.y, { component.Offset.x, component.Offset.y }, component.Rotation);
		}

		void BoxCollider2DComponent_GetOffset(uint64_t entityID, glm::vec2* out_Offset)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();
			*out_Offset = component.Offset;
		}

		void BoxCollider2DComponent_SetOffset(uint64_t entityID, glm::vec2* offset)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();
			const auto& transform = entity.Transform();
			component.Offset = *offset;

			SK_CORE_VERIFY(component.RuntimeCollider);
			SK_CORE_VERIFY(component.RuntimeCollider->GetType() == b2Shape::e_polygon);
			b2PolygonShape* shape = (b2PolygonShape*)component.RuntimeCollider->GetShape();
			shape->SetAsBox(component.Size.x * transform.Scale.x, component.Size.y * transform.Scale.y, { component.Offset.x, component.Offset.y }, component.Rotation);
		}

		float BoxCollider2DComponent_GetRotation(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();
			return component.Rotation;
		}

		void BoxCollider2DComponent_SetRotation(uint64_t entityID, float rotation)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<BoxCollider2DComponent>());

			auto& component = entity.GetComponent<BoxCollider2DComponent>();
			const auto& transform = entity.Transform();
			component.Rotation = rotation;

			SK_CORE_VERIFY(component.RuntimeCollider);
			SK_CORE_VERIFY(component.RuntimeCollider->GetType() == b2Shape::e_polygon);
			b2PolygonShape* shape = (b2PolygonShape*)component.RuntimeCollider->GetShape();
			shape->SetAsBox(component.Size.x * transform.Scale.x, component.Size.y * transform.Scale.y, { component.Offset.x, component.Offset.y }, component.Rotation);
		}

		#pragma endregion

		#pragma region CircleCollider2DComponent

		Coral::Bool32 CircleCollider2DComponent_IsSensor(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();

			SK_CORE_VERIFY(component.RuntimeCollider);
			return component.RuntimeCollider->IsSensor();
		}

		void CircleCollider2DComponent_SetSensor(uint64_t entityID, bool sensor)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();
			component.IsSensor = sensor;

			SK_CORE_VERIFY(component.RuntimeCollider);
			component.RuntimeCollider->SetSensor(sensor);
		}

		float CircleCollider2DComponent_GetDensity(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();

			SK_CORE_VERIFY(component.RuntimeCollider);
			return component.RuntimeCollider->GetDensity();
		}

		void CircleCollider2DComponent_SetDensity(uint64_t entityID, float density)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();
			component.Density = density;

			SK_CORE_VERIFY(component.RuntimeCollider);
			component.RuntimeCollider->SetDensity(density);
		}

		float CircleCollider2DComponent_GetFriction(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();

			SK_CORE_VERIFY(component.RuntimeCollider);
			return component.RuntimeCollider->GetFriction();
		}

		void CircleCollider2DComponent_SetFriction(uint64_t entityID, float friction)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();
			component.Friction = friction;

			SK_CORE_VERIFY(component.RuntimeCollider);
			component.RuntimeCollider->SetFriction(friction);
		}

		float CircleCollider2DComponent_GetRestitution(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();

			SK_CORE_VERIFY(component.RuntimeCollider);
			return component.RuntimeCollider->GetRestitution();
		}

		void CircleCollider2DComponent_SetRestitution(uint64_t entityID, float restitution)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();
			component.Restitution = restitution;

			SK_CORE_VERIFY(component.RuntimeCollider);
			component.RuntimeCollider->SetRestitution(restitution);
		}

		float CircleCollider2DComponent_GetRestitutionThreshold(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();

			SK_CORE_VERIFY(component.RuntimeCollider);
			return component.RuntimeCollider->GetRestitutionThreshold();
		}

		void CircleCollider2DComponent_SetRestitutionThreshold(uint64_t entityID, float restitutionThreshold)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();
			component.RestitutionThreshold = restitutionThreshold;

			SK_CORE_VERIFY(component.RuntimeCollider);
			component.RuntimeCollider->SetRestitutionThreshold(restitutionThreshold);
		}

		float CircleCollider2DComponent_GetRadius(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();
			return component.Radius;
		}

		void CircleCollider2DComponent_SetRadius(uint64_t entityID, float Radius)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();
			const auto& transform = entity.Transform();
			component.Radius = Radius;

			SK_CORE_VERIFY(component.RuntimeCollider);
			SK_CORE_VERIFY(component.RuntimeCollider->GetType() == b2Shape::e_circle);
			b2CircleShape* shape = (b2CircleShape*)component.RuntimeCollider->GetShape();
			shape->m_radius = component.Radius * transform.Scale.x;
			shape->m_p = { component.Offset.x, component.Offset.y };
		}

		void CircleCollider2DComponent_GetOffset(uint64_t entityID, glm::vec2* out_Offsets)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();
			*out_Offsets = component.Offset;
		}

		void CircleCollider2DComponent_SetOffset(uint64_t entityID, glm::vec2* offset)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();
			const auto& transform = entity.Transform();
			component.Offset = *offset;

			SK_CORE_VERIFY(component.RuntimeCollider);
			SK_CORE_VERIFY(component.RuntimeCollider->GetType() == b2Shape::e_circle);
			b2CircleShape* shape = (b2CircleShape*)component.RuntimeCollider->GetShape();
			shape->m_radius = component.Radius * transform.Scale.x;
			shape->m_p = { component.Offset.x, component.Offset.y };
		}

		float CircleCollider2DComponent_GetRotation(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();
			return component.Rotation;
		}

		void CircleCollider2DComponent_SetRotation(uint64_t entityID, float rotation)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<CircleCollider2DComponent>());

			auto& component = entity.GetComponent<CircleCollider2DComponent>();
			const auto& transform = entity.Transform();
			component.Rotation = rotation;

			SK_CORE_VERIFY(component.RuntimeCollider);
			SK_CORE_VERIFY(component.RuntimeCollider->GetType() == b2Shape::e_circle);
			b2CircleShape* shape = (b2CircleShape*)component.RuntimeCollider->GetShape();
			shape->m_radius = component.Radius * transform.Scale.x;
			shape->m_p = { component.Offset.x, component.Offset.y };
		}

		#pragma endregion

		#pragma region ScriptComponent

		Coral::ManagedObject ScriptComponent_GetInstance(uint64_t entityID)
		{
			Entity entity = GetEntity(entityID);
			SK_ICALL_VERIFY_PARAMETER(entity);
			SK_ICALL_VERIFY_PARAMETER(entity.HasComponent<ScriptComponent>());

			auto& component = entity.GetComponent<ScriptComponent>();
			SK_CORE_VERIFY(component.Instance);
			if (!component.OnCreateCalled)
			{
				component.Instance->InvokeMethod("OnCreate");
				component.OnCreateCalled = true;
			}

			return *component.Instance;
		}

		#pragma endregion


	}

}
