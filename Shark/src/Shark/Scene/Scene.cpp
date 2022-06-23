#include "skpch.h"
#include "Scene.h"

#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"
#include "Shark/Asset/ResourceManager.h"
#include "Shark/Render/SceneRenderer.h"

#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scripting/ScriptManager.h"
#include "Shark/Scripting/MonoGlue.h"

#include "Shark/Math/Math.h"

#include "Shark/Debug/enttDebug.h"
#include "Shark/Debug/Instrumentor.h"
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
			case RigidBody2DComponent::BodyType::Static:      return b2_staticBody;
			case RigidBody2DComponent::BodyType::Dynamic:     return b2_dynamicBody;
			case RigidBody2DComponent::BodyType::Kinematic:   return b2_kinematicBody;
		}

		SK_CORE_ASSERT(false, "Unkown Body Type");
		return b2_staticBody;
	}


	Scene::Scene()
	{
		SK_PROFILE_FUNCTION();
	}

	Scene::~Scene()
	{
		SK_PROFILE_FUNCTION();
	}

	Ref<Scene> Scene::Copy(Ref<Scene> srcScene)
	{
		SK_PROFILE_FUNCTION();
		
		auto newScene = Ref<Scene>::Create();
		newScene->m_ViewportWidth = srcScene->m_ViewportWidth;
		newScene->m_ViewportHeight = srcScene->m_ViewportHeight;
		newScene->m_ActiveCameraUUID = srcScene->m_ActiveCameraUUID;

		auto& srcRegistry = srcScene->m_Registry;
		auto& destRegistry = newScene->m_Registry;
		
		auto view = srcRegistry.view<IDComponent>();
		for (auto e : view)
		{
			Entity srcEntity{ e, srcScene };

			UUID uuid = srcEntity.GetUUID();
			newScene->m_EntityUUIDMap[uuid] = newScene->CreateEntityWithUUID(uuid, srcEntity.GetName());
		}

		CopyComponents<TransformComponent>(srcRegistry, destRegistry, newScene->m_EntityUUIDMap);
		CopyComponents<RelationshipComponent>(srcRegistry, destRegistry, newScene->m_EntityUUIDMap);
		CopyComponents<SpriteRendererComponent>(srcRegistry, destRegistry, newScene->m_EntityUUIDMap);
		CopyComponents<CircleRendererComponent>(srcRegistry, destRegistry, newScene->m_EntityUUIDMap);
		CopyComponents<CameraComponent>(srcRegistry, destRegistry, newScene->m_EntityUUIDMap);
		CopyComponents<NativeScriptComponent>(srcRegistry, destRegistry, newScene->m_EntityUUIDMap);
		CopyComponents<RigidBody2DComponent>(srcRegistry, destRegistry, newScene->m_EntityUUIDMap);
		CopyComponents<BoxCollider2DComponent>(srcRegistry, destRegistry, newScene->m_EntityUUIDMap);
		CopyComponents<CircleCollider2DComponent>(srcRegistry, destRegistry, newScene->m_EntityUUIDMap);
		CopyComponents<ScriptComponent>(srcRegistry, destRegistry, newScene->m_EntityUUIDMap);

		return newScene;
	}

	void Scene::OnScenePlay()
	{
		SK_PROFILE_FUNCTION();
		
		SetupBox2D();

		m_Registry.on_construct<RigidBody2DComponent>().connect<&Scene::OnRigidBody2DComponentCreated>(this);
		m_Registry.on_construct<BoxCollider2DComponent>().connect<&Scene::OnBoxCollider2DComponentCreated>(this);
		m_Registry.on_construct<CircleCollider2DComponent>().connect<&Scene::OnCircleCollider2DComponentCreated>(this);

		// Create Scripts
		{
			SK_PROFILE_SCOPED("Scene::OnScenePlay::InstantiateScripts");

			{
				auto view = m_Registry.view<ScriptComponent>();
				for (auto entityID : view)
				{
					Entity entity{ entityID, this };
					if (ScriptManager::Instantiate(entity))
					{
						auto& comp = view.get<ScriptComponent>(entityID);
						comp.HasRuntime = true;
					}
					else
					{
						// Remove Script Component if the Script couldn't be instatiated;
						entity.RemoveComponent<ScriptComponent>();
					}
				}
			}

			{
				auto view = m_Registry.view<ScriptComponent>();
				for (auto entityID : view)
				{
					Entity entity{ entityID, this };
					auto& script = ScriptManager::GetScript(entity.GetUUID());
					script.OnCreate();
				}
			}
		}

		// Create Native Scrips
		{
			auto view = m_Registry.view<NativeScriptComponent>();
			for (auto entityID : view)
			{
				auto& comp = view.get<NativeScriptComponent>(entityID);
				Entity entity{ entityID, this };
				if (comp.CreateScript)
				{
					comp.Script = comp.CreateScript(entity);
					comp.Script->OnCreate();
				}
			}
		}

		// Setup Cameras
		bool activeCameraFound = false;
		if (m_ActiveCameraUUID.IsValid())
		{
			Entity activeCamera = GetEntityByUUID(m_ActiveCameraUUID);
			if (activeCamera.AllOf<CameraComponent>())
			{
				m_RuntimeCamera = activeCamera;
				activeCameraFound = true;
			}
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
				cameraEntity.AddComponent<CameraComponent>();
			}

			m_ActiveCameraUUID = cameraEntity.GetUUID();
			m_RuntimeCamera = cameraEntity;
		}
		ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight);
	}

	void Scene::OnSceneStop()
	{
		SK_PROFILE_FUNCTION();

		m_Registry.on_construct<RigidBody2DComponent>().disconnect<&Scene::OnRigidBody2DComponentCreated>(this);
		m_Registry.on_construct<BoxCollider2DComponent>().disconnect<&Scene::OnBoxCollider2DComponentCreated>(this);
		m_Registry.on_construct<CircleCollider2DComponent>().disconnect<&Scene::OnCircleCollider2DComponentCreated>(this);

		// Destroy Scripts
		{
			SK_PROFILE_SCOPED("Scene::OnScenePlay::DestroyScripts");

			auto view = m_Registry.view<ScriptComponent>();

			for (auto entityID : view)
			{
				Entity entity{ entityID, this };
				auto& comp = entity.GetComponent<ScriptComponent>();
				if (comp.HasRuntime)
				{
					ScriptManager::Destroy(entity, true);
					comp.HasRuntime = false;
				}
			}

			ScriptManager::Cleanup();
		}

		// Destroy Native Scripts
		{
			auto view = m_Registry.view<NativeScriptComponent>();
			for (auto entityID : view)
			{
				auto& comp = view.get<NativeScriptComponent>(entityID);
				if (comp.Script)
				{
					comp.Script->OnDestroy();
					comp.DestroyScript(comp.Script);
				}
			}
		}
		
		m_PhysicsScene.DestoryScene();
		m_ContactListener.SetContext(nullptr);

		m_RuntimeCamera = entt::null;
	}

	void Scene::OnSimulateStart()
	{
		SK_PROFILE_FUNCTION();
		
		SetupBox2D();
	}

	void Scene::OnUpdateRuntime(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Scene::OnUpdateRuntime");

		{
			SK_PROFILE_SCOPED("Scene::OnScenePlay::UpdateScripts");

			auto view = m_Registry.view<ScriptComponent>();
			for (auto entityID : view)
			{
				Entity entity{ entityID, this };
				auto& script = ScriptManager::GetScript(entity.GetUUID());
				script.OnUpdate(ts);
			}
		}

		{
			auto view = m_Registry.view<NativeScriptComponent>();
			for (auto entityID : view)
			{
				auto& nsc = view.get<NativeScriptComponent>(entityID);
				if (nsc.Script)
					nsc.Script->OnUpdate(ts);
			}
		}

		{
			m_PhysicsScene.Step(ts);

			auto view = m_Registry.view<RigidBody2DComponent>();
			for (auto e : view)
			{
				Entity entity{ e, this };
				const auto& rb2d = view.get<RigidBody2DComponent>(entity);
				auto& transform = entity.Transform();
				const auto& pos = rb2d.RuntimeBody->GetPosition();
				transform.Translation.x = pos.x;
				transform.Translation.y = pos.y;
				transform.Rotation.z = rb2d.RuntimeBody->GetAngle();
			}
		}
	}

	void Scene::OnUpdateEditor(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();
	}

	void Scene::OnSimulate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();
		
		m_PhysicsScene.Step(ts);

		auto view = m_Registry.view<RigidBody2DComponent>();
		for (auto e : view)
		{
			Entity entity{ e, this };
			const auto& rb2d = view.get<RigidBody2DComponent>(entity);
			auto& transform = entity.Transform();
			const auto& pos = rb2d.RuntimeBody->GetPosition();
			transform.Translation.x = pos.x;
			transform.Translation.y = pos.y;
			transform.Rotation.z = rb2d.RuntimeBody->GetAngle();
		}
	}

	void Scene::OnRenderRuntimePreview(Ref<SceneRenderer> renderer, const glm::mat4& viewProj)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Scene::OnRenderRuntimePreview");

		renderer->SetScene(this);
		renderer->BeginScene(viewProj);

		{
			auto view = m_Registry.view<TransformComponent>();
			for (auto ent : view)
			{
				Entity entity{ ent, this };
				if (entity.HasParent())
					continue;

				RenderEntity(renderer, entity, glm::mat4(1.0f));
			}
		}

		renderer->EndScene();
	}

	void Scene::OnRenderRuntime(Ref<SceneRenderer> renderer)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Scene::OnRenderRuntime");

		Entity camerEntity = Entity{ m_RuntimeCamera, this };
		Camera& camera = camerEntity.GetComponent<CameraComponent>().Camera;
		auto& tf = camerEntity.Transform();

		renderer->SetScene(this);

		const auto viewProj = camera.GetProjection() * glm::inverse(tf.CalcTransform());
		renderer->BeginScene(viewProj);

		{
			auto view = m_Registry.view<TransformComponent>();
			for (auto ent : view)
			{
				Entity entity{ ent, this };
				if (entity.HasParent())
					continue;

				RenderEntity(renderer, entity, glm::mat4(1.0f));
			}
		}

		renderer->EndScene();
	}

	void Scene::OnRenderEditor(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Scene::OnRenderEditor");

		renderer->SetScene(this);
		renderer->BeginScene(editorCamera.GetViewProjection());

		{
			auto view = m_Registry.view<TransformComponent>();
			for (auto ent : view)
			{
				Entity entity{ ent, this };
				if (entity.HasParent())
					continue;

				RenderEntity(renderer, entity, glm::mat4(1.0f));
			}
		}

		renderer->EndScene();
	}

	void Scene::OnRenderSimulate(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Scene::OnRenderSimulate");

		renderer->SetScene(this);
		renderer->BeginScene(editorCamera.GetViewProjection());

		{
			auto view = m_Registry.view<TransformComponent>();
			for (auto ent : view)
			{
				Entity entity{ ent, this };
				if (entity.HasParent())
					continue;

				RenderEntity(renderer, entity, glm::mat4(1.0f));
			}
		}

		renderer->EndScene();
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
		CopyComponentIfExists<NativeScriptComponent>(srcEntity, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
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

		if (m_EntityUUIDMap.find(uuid) != m_EntityUUIDMap.end())
		{


			SK_DEBUG_BREAK();
		}

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

	void Scene::DestroyAllEntities()
	{
		SK_PROFILE_FUNCTION();

		m_Registry.clear();
		ScriptManager::Cleanup();
		m_PhysicsScene.DestroyAllBodies();
		m_EntityUUIDMap.clear();
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
			if (entity.AllOf<ScriptComponent>())
				ScriptManager::Destroy(entity, true);

			if (entity.AllOf<RigidBody2DComponent>())
			{
				auto& rb = entity.GetComponent<RigidBody2DComponent>();
				SK_CORE_ASSERT(rb.RuntimeBody);
				m_PhysicsScene.GetWorld()->DestroyBody(rb.RuntimeBody);
			}
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
		SK_PROFILE_FUNCTION();
		
		if (m_EntityUUIDMap.find(uuid) != m_EntityUUIDMap.end())
			return m_EntityUUIDMap.at(uuid);
		return Entity{};
	}

	bool Scene::IsValidEntity(Entity entity)const
	{
		SK_PROFILE_FUNCTION();
		
		return m_Registry.valid(entity);
	}

	Entity Scene::GetActiveCameraEntity() const
	{
		return GetEntityByUUID(m_ActiveCameraUUID);
	}

	Entity Scene::GetRuntimeCamera()
	{
		SK_PROFILE_FUNCTION();
		
		SK_CORE_ASSERT(m_RuntimeCamera != entt::null);
		return Entity{ m_RuntimeCamera, this };
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

	void Scene::SetupBox2D()
	{
		SK_PROFILE_FUNCTION();
		
		m_ContactListener.SetContext(this);
		m_PhysicsScene.CreateScene();
		b2World* world = m_PhysicsScene.GetWorld();
		world->SetContactListener(&m_ContactListener);

		// Add missing RigidBodys
		{
			auto view = m_Registry.view<BoxCollider2DComponent>();
			for (auto entity : view)
			{
				if (!m_Registry.all_of<RigidBody2DComponent>(entity))
				{
					auto& rb = m_Registry.emplace<RigidBody2DComponent>(entity);
					rb.Type = RigidBody2DComponent::BodyType::Static;
				}
			}
		}

		// Add missing RigidBodys
		{
			auto view = m_Registry.view<CircleCollider2DComponent>();
			for (auto entity : view)
			{
				if (!m_Registry.all_of<RigidBody2DComponent>(entity))
				{
					auto& rb = m_Registry.emplace<RigidBody2DComponent>(entity);
					rb.Type = RigidBody2DComponent::BodyType::Static;
				}
			}
		}

		// Setup Box2D side
		{
			auto view = m_Registry.view<RigidBody2DComponent>();
			for (auto e : view)
			{
				Entity entity{ e, this };
				auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
				auto& transform = entity.Transform();

				b2BodyDef bodydef;
				bodydef.type = SharkBodyTypeToBox2D(rb2d.Type);
				bodydef.position = { transform.Translation.x, transform.Translation.y };
				bodydef.angle = transform.Rotation.z;
				bodydef.fixedRotation = rb2d.FixedRotation;
				bodydef.bullet = rb2d.IsBullet;
				bodydef.awake = rb2d.Awake;
				bodydef.enabled = rb2d.Enabled;
				bodydef.gravityScale = rb2d.GravityScale;
				bodydef.allowSleep = rb2d.AllowSleep;
				bodydef.userData.pointer = (uintptr_t)(uint64_t)entity.GetUUID();

				rb2d.RuntimeBody = world->CreateBody(&bodydef);

				if (entity.AllOf<BoxCollider2DComponent>())
				{
					auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

					b2PolygonShape shape;
					shape.SetAsBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y, { bc2d.Offset.x, bc2d.Offset.y }, bc2d.Rotation);

					b2FixtureDef fixturedef;
					fixturedef.shape = &shape;
					fixturedef.friction = bc2d.Friction;
					fixturedef.density = bc2d.Density;
					fixturedef.restitution = bc2d.Restitution;
					fixturedef.restitutionThreshold = bc2d.RestitutionThreshold;
					fixturedef.isSensor = bc2d.IsSensor;

					bc2d.RuntimeCollider = rb2d.RuntimeBody->CreateFixture(&fixturedef);
				}

				if (entity.AllOf<CircleCollider2DComponent>())
				{
					auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

					b2CircleShape shape;
					shape.m_radius = cc2d.Radius * transform.Scale.x;
					shape.m_p = { cc2d.Offset.x, cc2d.Offset.y };

					b2FixtureDef fixturedef;
					fixturedef.shape = &shape;
					fixturedef.friction = cc2d.Friction;
					fixturedef.density = cc2d.Density;
					fixturedef.restitution = cc2d.Restitution;
					fixturedef.restitutionThreshold = cc2d.RestitutionThreshold;
					fixturedef.isSensor = cc2d.IsSensor;

					cc2d.RuntimeCollider = rb2d.RuntimeBody->CreateFixture(&fixturedef);
				}
			}
		}
	}

	void Scene::RenderEntity(const Ref<SceneRenderer>& renderer, Entity entity, const glm::mat4& parentTransform)
	{
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

	void Scene::OnRigidBody2DComponentCreated(entt::registry& registry, entt::entity entityID)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ entityID, this };
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
		auto& transform = entity.Transform();

		b2BodyDef bodydef;
		bodydef.type = SharkBodyTypeToBox2D(rb2d.Type);
		bodydef.position = { transform.Translation.x, transform.Translation.y };
		bodydef.angle = transform.Rotation.z;
		bodydef.fixedRotation = rb2d.FixedRotation;
		bodydef.userData.pointer = (uintptr_t)(uint64_t)entity.GetUUID();

		rb2d.RuntimeBody = m_PhysicsScene.GetWorld()->CreateBody(&bodydef);
	}

	void Scene::OnBoxCollider2DComponentCreated(entt::registry& registry, entt::entity entityID)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ entityID, this };
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
		auto& transform = entity.Transform();
		auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

		b2PolygonShape shape;
		shape.SetAsBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y, { bc2d.Offset.x, bc2d.Offset.y }, bc2d.Rotation);

		b2FixtureDef fixturedef;
		fixturedef.shape = &shape;
		fixturedef.friction = bc2d.Friction;
		fixturedef.density = bc2d.Density;
		fixturedef.restitution = bc2d.Restitution;
		fixturedef.restitutionThreshold = bc2d.RestitutionThreshold;

		bc2d.RuntimeCollider = rb2d.RuntimeBody->CreateFixture(&fixturedef);
	}

	void Scene::OnCircleCollider2DComponentCreated(entt::registry& registry, entt::entity entityID)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ entityID, this };
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
		auto& transform = entity.Transform();
		auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

		b2CircleShape shape;
		shape.m_radius = cc2d.Radius * transform.Scale.x;
		shape.m_p = { cc2d.Offset.x, cc2d.Offset.y };

		b2FixtureDef fixturedef;
		fixturedef.shape = &shape;
		fixturedef.friction = cc2d.Friction;
		fixturedef.density = cc2d.Density;
		fixturedef.restitution = cc2d.Restitution;
		fixturedef.restitutionThreshold = cc2d.RestitutionThreshold;

		cc2d.RuntimeCollider = rb2d.RuntimeBody->CreateFixture(&fixturedef);
	}

	void ContactListener::BeginContact(b2Contact* contact)
	{
		b2Fixture* fixtureA = contact->GetFixtureA();
		b2Fixture* fixtureB = contact->GetFixtureB();

		b2Body* bodyA = fixtureA->GetBody();
		b2Body* bodyB = fixtureB->GetBody();

		UUID uuidA = (UUID)bodyA->GetUserData().pointer;
		UUID uuidB = (UUID)bodyB->GetUserData().pointer;

		Entity entityA = m_Context->GetEntityByUUID(uuidA);
		Entity entityB = m_Context->GetEntityByUUID(uuidB);

		MonoGlue::CallCollishionBegin(entityA, entityB);
	}

	void ContactListener::EndContact(b2Contact* contact)
	{
		b2Fixture* fixtureA = contact->GetFixtureA();
		b2Fixture* fixtureB = contact->GetFixtureB();

		b2Body* bodyA = fixtureA->GetBody();
		b2Body* bodyB = fixtureB->GetBody();

		UUID uuidA = (UUID)bodyA->GetUserData().pointer;
		UUID uuidB = (UUID)bodyB->GetUserData().pointer;

		Entity entityA = m_Context->GetEntityByUUID(uuidA);
		Entity entityB = m_Context->GetEntityByUUID(uuidB);

		MonoGlue::CallCollishionEnd(entityA, entityB);
	}

	void ContactListener::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
	}

}
