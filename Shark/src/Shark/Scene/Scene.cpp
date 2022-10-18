#include "skpch.h"
#include "Scene.h"

#include "Shark/Scene/Entity.h"
#include "Shark/Asset/ResourceManager.h"
#include "Shark/Render/SceneRenderer.h"

#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scripting/ScriptGlue.h"

#include "Shark/Physics2D/Physics2D.h"
#include "Shark/Physics2D/PhysicsScene.h"

#include "Shark/Math/Math.h"

#include "Shark/Debug/enttDebug.h"
#include "Shark/Debug/Profiler.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>
#include <box2d/b2_fixture.h>
#include "box2d/b2_contact.h"

namespace Shark {

	template<typename Component>
	void CopyComponents(entt::registry& srcRegistry, entt::registry& destRegistry, const std::unordered_map<UUID, Entity>& entityUUIDMap)
	{
		SK_PROFILE_FUNCTION();

		auto view = srcRegistry.view<Component>();
		for (auto srcEntity : view)
		{
			Entity destEntity = entityUUIDMap.at(srcRegistry.get<IDComponent>(srcEntity).ID);

			auto& comp = srcRegistry.get<Component>(srcEntity);
			destRegistry.emplace_or_replace<Component>(destEntity, comp);
		}
	}

	template<typename Component>
	void CopyComponentIfExists(entt::entity srcEntity, entt::registry& srcRegistry, entt::entity destEntity, entt::registry& destRegistry)
	{
		SK_PROFILE_FUNCTION();

		if (auto* comp = srcRegistry.try_get<Component>(srcEntity))
			destRegistry.emplace_or_replace<Component>(destEntity, *comp);
	}

	static b2BodyType SharkBodyTypeToBox2D(RigidBody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
			case RigidBody2DComponent::BodyType::None:        return b2_staticBody; // use static body as default fallback
			case RigidBody2DComponent::BodyType::Static:      return b2_staticBody;
			case RigidBody2DComponent::BodyType::Dynamic:     return b2_dynamicBody;
			case RigidBody2DComponent::BodyType::Kinematic:   return b2_kinematicBody;
		}

		SK_CORE_ASSERT(false, "Unkown Body Type");
		return b2_staticBody;
	}


	Scene::Scene()
	{
		m_Registry.on_destroy<CameraComponent>().connect<&Scene::OnCameraComponentDestroyed>(this);
	}

	Scene::~Scene()
	{
		m_Registry.on_destroy<CameraComponent>().disconnect<&Scene::OnCameraComponentDestroyed>(this);
	}

	Ref<Scene> Scene::Copy(Ref<Scene> srcScene)
	{
		Ref<Scene> newScene = Ref<Scene>::Create();
		srcScene->CopyTo(newScene);
		return newScene;
	}

	void Scene::CopyTo(Ref<Scene> destScene)
	{
		SK_PROFILE_FUNCTION();

		destScene->m_ViewportWidth = m_ViewportWidth;
		destScene->m_ViewportHeight = m_ViewportHeight;
		destScene->m_ActiveCameraUUID = m_ActiveCameraUUID;

		auto& destRegistry = destScene->m_Registry;

		auto view = m_Registry.view<IDComponent>();
		for (auto e : view)
		{
			Entity srcEntity{ e, this };

			UUID uuid = srcEntity.GetUUID();
			destScene->m_EntityUUIDMap[uuid] = destScene->CreateEntityWithUUID(uuid, srcEntity.GetName());
		}

		CopyComponents<TransformComponent>(m_Registry, destRegistry, destScene->m_EntityUUIDMap);
		CopyComponents<RelationshipComponent>(m_Registry, destRegistry, destScene->m_EntityUUIDMap);
		CopyComponents<SpriteRendererComponent>(m_Registry, destRegistry, destScene->m_EntityUUIDMap);
		CopyComponents<CircleRendererComponent>(m_Registry, destRegistry, destScene->m_EntityUUIDMap);
		CopyComponents<CameraComponent>(m_Registry, destRegistry, destScene->m_EntityUUIDMap);
		CopyComponents<RigidBody2DComponent>(m_Registry, destRegistry, destScene->m_EntityUUIDMap);
		CopyComponents<BoxCollider2DComponent>(m_Registry, destRegistry, destScene->m_EntityUUIDMap);
		CopyComponents<CircleCollider2DComponent>(m_Registry, destRegistry, destScene->m_EntityUUIDMap);
		CopyComponents<ScriptComponent>(m_Registry, destRegistry, destScene->m_EntityUUIDMap);

	}

	void Scene::OnScenePlay()
	{
		SK_PROFILE_FUNCTION();
		
		m_IsRunning = true;

		Physics2D::CreateScene(this);

		m_Registry.on_construct<RigidBody2DComponent>().connect<&Scene::OnRigidBody2DComponentCreated>(this);
		m_Registry.on_construct<BoxCollider2DComponent>().connect<&Scene::OnBoxCollider2DComponentCreated>(this);
		m_Registry.on_construct<CircleCollider2DComponent>().connect<&Scene::OnCircleCollider2DComponentCreated>(this);

		m_Registry.on_destroy<RigidBody2DComponent>().connect<&Scene::OnRigidBody2DComponentDestroyed>(this);
		m_Registry.on_destroy<BoxCollider2DComponent>().connect<&Scene::OnBoxCollider2DComponentDestroyed>(this);
		m_Registry.on_destroy<CircleCollider2DComponent>().connect<&Scene::OnCircleCollider2DComponentDestroyed>(this);
		m_Registry.on_destroy<ScriptComponent>().connect<&Scene::OnScriptComponentDestroyed>(this);

		// Create Scripts
		{
			SK_PROFILE_SCOPED("Scene::OnScenePlay::InstantiateScripts");

			{
				auto view = m_Registry.view<ScriptComponent>();
				for (auto entityID : view)
				{
					Entity entity{ entityID, this };
					ScriptEngine::InstantiateEntity(entity, false, false);
				}

				for (auto entityID : view)
				{
					Entity entity{ entityID, this };
					ScriptEngine::InitializeFields(entity);
				}
			}

			{

				for (const auto& [uuid, gcHandle] : ScriptEngine::GetEntityInstances())
					MethodThunks::OnCreate(gcHandle);
			}
		}

		// Setup Cameras
		bool activeCameraFound = false;
		if (m_ActiveCameraUUID.IsValid())
		{
			Entity activeCamera = GetEntityByUUID(m_ActiveCameraUUID);
			if (activeCamera.AllOf<CameraComponent>())
				activeCameraFound = true;
		}

		if (!activeCameraFound)
		{
			Entity cameraEntity;
			auto view = m_Registry.view<CameraComponent>();
			if (!view.empty())
			{
				cameraEntity = Entity{ view.front(), this };
			}
			else
			{
				cameraEntity = CreateEntity("Fallback Camera");
				cameraEntity.Transform().Translation.z = -10.0f;
				cameraEntity.AddComponent<CameraComponent>();
			}

			m_ActiveCameraUUID = cameraEntity.GetUUID();
		}
		ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight);
	}

	void Scene::OnSceneStop()
	{
		SK_PROFILE_FUNCTION();

		m_IsRunning = false;

		m_Registry.on_construct<RigidBody2DComponent>().disconnect<&Scene::OnRigidBody2DComponentCreated>(this);
		m_Registry.on_construct<BoxCollider2DComponent>().disconnect<&Scene::OnBoxCollider2DComponentCreated>(this);
		m_Registry.on_construct<CircleCollider2DComponent>().disconnect<&Scene::OnCircleCollider2DComponentCreated>(this);

		m_Registry.on_destroy<RigidBody2DComponent>().disconnect<&Scene::OnRigidBody2DComponentDestroyed>(this);
		m_Registry.on_destroy<BoxCollider2DComponent>().disconnect<&Scene::OnBoxCollider2DComponentDestroyed>(this);
		m_Registry.on_destroy<CircleCollider2DComponent>().disconnect<&Scene::OnCircleCollider2DComponentDestroyed>(this);
		m_Registry.on_destroy<ScriptComponent>().disconnect<&Scene::OnScriptComponentDestroyed>(this);

		// Destroy Scripts
		{
			SK_PROFILE_SCOPED("Scene::OnScenePlay::DestroyScripts");

			for (auto& [uuid, gcHandle] : ScriptEngine::GetEntityInstances())
				MethodThunks::OnDestroy(gcHandle);

			ScriptEngine::ShutdownRuntime();
		}

		Physics2D::ReleaseScene();
	}

	void Scene::OnSimulationPlay()
	{
		Physics2D::CreateScene(this);
	}

	void Scene::OnUpdateRuntime(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Scene::OnUpdateRuntime");

		{
			SK_PROFILE_SCOPED("Update Scripts");
			SK_PERF_SCOPED("Update Scripts");

			for (auto& [uuid, gcHandle] : ScriptEngine::GetEntityInstances())
				MethodThunks::OnUpdate(gcHandle, ts);
		}

		Ref<Physics2DScene> physicsScene = Physics2D::GetScene();
		physicsScene->Simulate(ts);

		for (const auto& fn : m_PostUpdateQueue)
			fn();
		m_PostUpdateQueue.clear();
	}

	void Scene::OnUpdateEditor(TimeStep ts)
	{
	}

	void Scene::OnUpdateSimulate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Scene::OnUpdateSimulate");

		Ref<Physics2DScene> physicsScene = Physics2D::GetScene();
		physicsScene->Simulate(ts);
	}

	void Scene::OnRenderRuntime(Ref<SceneRenderer> renderer)
	{
		Entity cameraEntity = GetEntityByUUID(m_ActiveCameraUUID);
		Camera& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
		auto& tf = cameraEntity.Transform();

		const auto viewProj = camera.GetProjection() * glm::inverse(GetWorldSpaceTransformMatrix(cameraEntity));
		OnRender(renderer, viewProj);
	}

	void Scene::OnRenderEditor(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera)
	{
		OnRender(renderer, editorCamera.GetViewProjection());
	}

	void Scene::OnRenderSimulate(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera)
	{
		OnRender(renderer, editorCamera.GetViewProjection());
	}

	void Scene::OnRender(Ref<SceneRenderer> renderer, const glm::mat4& viewProj)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Scene::OnRender");

		renderer->SetScene(this);
		renderer->BeginScene(viewProj);

		auto view = m_Registry.view<TransformComponent>();
		for (auto ent : view)
		{
			Entity entity{ ent, this };
			if (entity.HasParent())
				continue;

			RenderEntity(renderer, entity, glm::mat4(1.0f));
		}

		renderer->EndScene();
	}

	void Scene::OnFixedUpdate(float ts)
	{
		for (auto& [uuid, gcHandle] : ScriptEngine::GetEntityInstances())
			MethodThunks::OnPhysicsUpdate(gcHandle, ts);
	}

	Entity Scene::CloneEntity(Entity srcEntity)
	{
		SK_PROFILE_FUNCTION();
		
		Entity newEntity = CreateEntity();

		CopyComponentIfExists<TagComponent>(srcEntity, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<TransformComponent>(srcEntity, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<SpriteRendererComponent>(srcEntity, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<CircleRendererComponent>(srcEntity, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<CameraComponent>(srcEntity, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<RigidBody2DComponent>(srcEntity, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<BoxCollider2DComponent>(srcEntity, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<CircleCollider2DComponent>(srcEntity, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<ScriptComponent>(srcEntity, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);

		return newEntity;
	}

	Entity Scene::CreateEntity(const std::string& tag)
	{
		SK_PROFILE_FUNCTION();
		
		return CreateEntityWithUUID(UUID::Generate(), tag);
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& tag)
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(uuid.IsValid());
		if (!uuid.IsValid())
			uuid = UUID::Generate();

		Entity entity{ m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TagComponent>(tag.empty() ? "new Entity" : tag);
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<RelationshipComponent>();

		SK_CORE_ASSERT(m_EntityUUIDMap.find(uuid) == m_EntityUUIDMap.end());
		m_EntityUUIDMap[uuid] = entity;
		return entity;
	}

	Entity Scene::CreateChildEntity(Entity parent, const std::string& tag)
	{
		return CreateChildEntityWithUUID(parent, UUID::Generate(), tag);
	}

	Entity Scene::CreateChildEntityWithUUID(Entity parent, UUID uuid, const std::string& tag)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(uuid.IsValid());
		if (!uuid.IsValid())
			uuid = UUID::Generate();

		Entity entity = CreateEntityWithUUID(uuid, tag);

		if (parent)
			entity.SetParent(parent);

		return entity;
	}

	void Scene::DestroyEntity(Entity entity, bool destroyChildren)
	{
		DestroyEntityInternal(entity, destroyChildren, true);
	}

	void Scene::DestroyEntityInternal(Entity entity, bool destroyChildren, bool first)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(entity);
		if (!entity)
			return;

		SK_CORE_ASSERT(m_EntityUUIDMap.find(entity.GetUUID()) != m_EntityUUIDMap.end());

		if (!m_IsEditorScene)
		{
			if (entity.AllOf<RigidBody2DComponent>())
			{
				auto physicsScene = Physics2D::GetScene();
				physicsScene->DestroyActor(entity);
			}

			if (entity.AllOf<ScriptComponent>())
				ScriptEngine::OnEntityDestroyed(entity);
		}

		if (first)
		{
			entity.RemoveParent();
			entity.RemoveChildren();
		}

		if (destroyChildren)
		{
			for (auto& childID : entity.Children())
			{
				Entity child = m_EntityUUIDMap.at(childID);
				DestroyEntityInternal(child, destroyChildren, false);
			}
		}

		const UUID uuid = entity.GetUUID();
		m_Registry.destroy(entity);
		m_EntityUUIDMap.erase(uuid);
	}

	Entity Scene::GetEntityByUUID(UUID uuid) const
	{
		if (m_EntityUUIDMap.find(uuid) != m_EntityUUIDMap.end())
			return m_EntityUUIDMap.at(uuid);
		return Entity{};
	}

	Entity Scene::FindEntityByTag(const std::string& tag)
	{
		SK_PROFILE_FUNCTION();

		auto view = m_Registry.view<TagComponent>();
		for (auto& ent : view)
		{
			auto& comp = view.get<TagComponent>(ent);
			if (comp.Tag == tag)
				return { ent, this };
		}
		return Entity{};
	}

	Entity Scene::FindChildEntityByName(Entity entity, const std::string& name, bool recusive)
	{
		for (UUID id : entity.Children())
		{
			Entity child = m_EntityUUIDMap.at(id);
			if (child.GetName() == name)
				return child;

			if (recusive)
				FindChildEntityByName(child, name, recusive);
		}

		return Entity{};
	}

	bool Scene::IsValidEntity(Entity entity)const
	{
		return m_Registry.valid(entity);
	}

	bool Scene::ValidEntityID(UUID entityID) const
	{
		return m_EntityUUIDMap.find(entityID) != m_EntityUUIDMap.end();
	}

	Entity Scene::GetActiveCameraEntity() const
	{
		return GetEntityByUUID(m_ActiveCameraUUID);
	}

	void Scene::ResizeCameras(float width, float height)
	{
		SK_PROFILE_FUNCTION();
		
		auto view = m_Registry.view<CameraComponent>();
		for (auto entityID : view)
		{
			auto& cc = view.get<CameraComponent>(entityID);
			cc.Camera.Resize(width, height);
		}
	}

	void Scene::SetViewportSize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		if (m_ViewportWidth != 0 && m_ViewportHeight != 0)
			ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight);
	}

	glm::mat4 Scene::GetWorldSpaceTransformMatrix(Entity entity) const
	{
		if (entity.HasParent())
			return GetWorldSpaceTransformMatrix(entity.Parent()) * entity.CalcTransform();
		return entity.CalcTransform();
	}

	TransformComponent Scene::GetWorldSpaceTransform(Entity entity)
	{
		if (!entity.HasParent())
			return entity.Transform();

		glm::mat4 worldSpaceTransform = GetWorldSpaceTransformMatrix(entity);
		TransformComponent transform;
		Math::DecomposeTransform(worldSpaceTransform, transform.Translation, transform.Rotation, transform.Scale);
		return transform;
	}

	bool Scene::ConvertToLocaSpace(Entity entity, glm::mat4& transformMatrix)
	{
		if (!entity.HasParent())
			return true;

		transformMatrix = glm::inverse(GetWorldSpaceTransformMatrix(entity.Parent())) * transformMatrix;
		return true;
	}

	bool Scene::ConvertToWorldSpace(Entity entity, glm::mat4& transformMatrix)
	{
		if (!entity.HasParent())
			return true;

		transformMatrix = GetWorldSpaceTransformMatrix(entity.Parent()) * transformMatrix;
		return true;
	}

	bool Scene::ConvertToLocaSpace(Entity entity, TransformComponent& transform)
	{
		if (!entity.HasParent())
			return true;

		glm::mat4 localTransformMatrix = glm::inverse(GetWorldSpaceTransformMatrix(entity.Parent())) * transform.CalcTransform();
		return Math::DecomposeTransform(localTransformMatrix, transform.Translation, transform.Rotation, transform.Scale);
	}

	bool Scene::ConvertToWorldSpace(Entity entity, TransformComponent& transform)
	{
		if (!entity.HasParent())
			return true;

		glm::mat4 worldTransformMatrix = GetWorldSpaceTransformMatrix(entity);
		return Math::DecomposeTransform(worldTransformMatrix, transform.Translation, transform.Rotation, transform.Scale);
	}

	bool Scene::ConvertToLocaSpace(Entity entity)
	{
		return ConvertToLocaSpace(entity, entity.Transform());
	}

	bool Scene::ConvertToWorldSpace(Entity entity)
	{
		return ConvertToWorldSpace(entity, entity.Transform());
	}

	void Scene::RenderEntity(const Ref<SceneRenderer>& renderer, Entity entity, const glm::mat4& parentTransform)
	{
		SK_PROFILE_FUNCTION();

		const glm::mat4 transform = parentTransform * entity.Transform().CalcTransform();

		if (entity.AllOf<SpriteRendererComponent>())
		{
			auto& sr = entity.GetComponent<SpriteRendererComponent>();
			renderer->SubmitQuad(transform, ResourceManager::GetAsset<Texture2D>(sr.TextureHandle), sr.TilingFactor, sr.Color, (int)entity.GetHandle());
		}

		if (entity.AllOf<CircleRendererComponent>())
		{
			auto& cr = entity.GetComponent<CircleRendererComponent>();
			renderer->SubmitCircle(transform, cr.Thickness, cr.Fade, cr.Color, (int)entity.GetHandle());
		}

		for (auto& childID : entity.Children())
			RenderEntity(renderer, GetEntityByUUID(childID), transform);
	}

	void Scene::OnRigidBody2DComponentCreated(entt::registry& registry, entt::entity ent)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ ent, this };
		Physics2D::GetScene()->CreateActor(entity);
	}

	void Scene::OnBoxCollider2DComponentCreated(entt::registry& registry, entt::entity ent)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ ent, this };
		Physics2D::GetScene()->OnColliderCreated(entity, entity.GetComponent<BoxCollider2DComponent>());
	}

	void Scene::OnCircleCollider2DComponentCreated(entt::registry& registry, entt::entity ent)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ ent, this };
		Physics2D::GetScene()->OnColliderCreated(entity, entity.GetComponent<CircleCollider2DComponent>());
	}

	void Scene::OnRigidBody2DComponentDestroyed(entt::registry& registry, entt::entity ent)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ ent, this };
		Physics2D::GetScene()->DestroyActor(entity);
	}

	void Scene::OnBoxCollider2DComponentDestroyed(entt::registry& registry, entt::entity ent)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ ent, this };
		Physics2D::GetScene()->OnColliderDestroyed(entity, entity.GetComponent<BoxCollider2DComponent>());
	}

	void Scene::OnCircleCollider2DComponentDestroyed(entt::registry& registry, entt::entity ent)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ ent, this };
		Physics2D::GetScene()->OnColliderDestroyed(entity, entity.GetComponent<CircleCollider2DComponent>());
	}

	void Scene::OnScriptComponentDestroyed(entt::registry& registry, entt::entity ent)
	{
		Entity entity{ ent, this };

		if (ScriptEngine::IsInstantiated(entity))
			ScriptEngine::DestroyEntityInstance(entity, true);
	}

	void Scene::OnCameraComponentDestroyed(entt::registry& registry, entt::entity ent)
	{
		Entity entity{ ent, this };

		if (!entity.AllOf<IDComponent>())
			return;

		UUID uuid = entity.GetUUID();
		if (uuid == m_ActiveCameraUUID)
			m_ActiveCameraUUID = UUID::Null;
	}

}
