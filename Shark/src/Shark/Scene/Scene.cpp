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
	void CopyComponents(entt::registry& srcRegistry, entt::registry& destRegistry, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		SK_PROFILE_FUNCTION();

		auto view = srcRegistry.view<Component>();
		for (auto srcEntity : view)
		{
			entt::entity destEntity = enttMap.at(srcRegistry.get<IDComponent>(srcEntity).ID);

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
		newScene->m_ViewportBounds = srcScene->m_ViewportBounds;
		//newScene->m_ViewportWidth = srcScene->m_ViewportWidth;
		//newScene->m_ViewportHeight = srcScene->m_ViewportHeight;
		newScene->m_ActiveCameraUUID = srcScene->m_ActiveCameraUUID;

		auto& srcRegistry = srcScene->m_Registry;
		auto& destRegistry = newScene->m_Registry;
		
		std::unordered_map<UUID, entt::entity> enttMap;

		auto view = srcRegistry.view<IDComponent>();
		for (auto e : view)
		{
			Entity srcEntity{ e, srcScene };

			UUID uuid = srcEntity.GetUUID();
			enttMap[uuid] = newScene->CreateEntityWithUUID(uuid, srcEntity.GetName());
		}

		CopyComponents<TransformComponent>(srcRegistry, destRegistry, enttMap);
		CopyComponents<SpriteRendererComponent>(srcRegistry, destRegistry, enttMap);
		CopyComponents<CircleRendererComponent>(srcRegistry, destRegistry, enttMap);
		CopyComponents<CameraComponent>(srcRegistry, destRegistry, enttMap);
		CopyComponents<NativeScriptComponent>(srcRegistry, destRegistry, enttMap);
		CopyComponents<RigidBody2DComponent>(srcRegistry, destRegistry, enttMap);
		CopyComponents<BoxCollider2DComponent>(srcRegistry, destRegistry, enttMap);
		CopyComponents<CircleCollider2DComponent>(srcRegistry, destRegistry, enttMap);
		CopyComponents<ScriptComponent>(srcRegistry, destRegistry, enttMap);

		return newScene;
	}

	void Scene::OnScenePlay()
	{
		SK_PROFILE_FUNCTION();
		
		SetupBox2D();

		m_Registry.on_construct<RigidBody2DComponent>().connect<&Scene::OnRigidBody2DComponentCreated>(this);
		m_Registry.on_construct<BoxCollider2DComponent>().connect<&Scene::OnBoxCollider2DComponentCreated>(this);
		m_Registry.on_construct<CircleCollider2DComponent>().connect<&Scene::OnCircleCollider2DComponentCreated>(this);

		m_Registry.on_destroy<RigidBody2DComponent>().connect<&Scene::OnRigidBody2DComponentDestroyed>(this);
		m_Registry.on_destroy<BoxCollider2DComponent>().connect<&Scene::OnBoxCollider2DComponentDestroyed>(this);
		m_Registry.on_destroy<CircleCollider2DComponent>().connect<&Scene::OnCircleCollider2DComponentDestroyed>(this);

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
			if (activeCamera.HasComponent<CameraComponent>())
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
		ResizeCameras((float)m_ViewportBounds.GetWidth(), (float)m_ViewportBounds.GetHeight());
	}

	void Scene::OnSceneStop()
	{
		SK_PROFILE_FUNCTION();

		m_Registry.on_construct<RigidBody2DComponent>().disconnect<&Scene::OnRigidBody2DComponentCreated>(this);
		m_Registry.on_construct<BoxCollider2DComponent>().disconnect<&Scene::OnBoxCollider2DComponentCreated>(this);
		m_Registry.on_construct<CircleCollider2DComponent>().disconnect<&Scene::OnCircleCollider2DComponentCreated>(this);

		m_Registry.on_destroy<RigidBody2DComponent>().disconnect<&Scene::OnRigidBody2DComponentDestroyed>(this);
		m_Registry.on_destroy<BoxCollider2DComponent>().disconnect<&Scene::OnBoxCollider2DComponentDestroyed>(this);
		m_Registry.on_destroy<CircleCollider2DComponent>().disconnect<&Scene::OnCircleCollider2DComponentDestroyed>(this);

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
				auto& transform = entity.GetTransform();
				const auto& pos = rb2d.RuntimeBody->GetPosition();
				transform.Position.x = pos.x;
				transform.Position.y = pos.y;
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
			auto& transform = entity.GetTransform();
			const auto& pos = rb2d.RuntimeBody->GetPosition();
			transform.Position.x = pos.x;
			transform.Position.y = pos.y;
			transform.Rotation.z = rb2d.RuntimeBody->GetAngle();
		}
	}

	void Scene::OnEventRuntime(Event& event)
	{
		// passes events to C#
		MonoGlue::OnEvent(event);
	}

	void Scene::OnRenderRuntimePreview(Ref<SceneRenderer> renderer, const glm::mat4& viewProj)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Scene::OnRenderRuntimePreview");

		renderer->SetScene(this);
		renderer->BeginScene(viewProj);

		{
			auto view = m_Registry.view<SpriteRendererComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto& [sr, tf] = view.get<SpriteRendererComponent, TransformComponent>(entity);
				Ref<Texture2D> texture;
				if (ResourceManager::IsValidAssetHandle(sr.TextureHandle))
					texture = ResourceManager::GetAsset<Texture2D>(sr.TextureHandle);
				renderer->SubmitQuad(tf.Position, tf.Rotation, tf.Scaling, texture, sr.TilingFactor, sr.Color, (int)entity);
			}
		}

		{
			auto view = m_Registry.view<CircleRendererComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto& [cr, tf] = view.get<CircleRendererComponent, TransformComponent>(entity);
				renderer->SubmitCirlce(tf.Position, tf.Rotation, tf.Scaling, cr.Color, cr.Thickness, cr.Fade, (int)entity);
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
		auto& tf = camerEntity.GetTransform();

		renderer->SetScene(this);

		const auto viewProj = camera.GetProjection() * glm::inverse(tf.GetTranform());
		renderer->BeginScene(viewProj);

		{
			auto view = m_Registry.view<SpriteRendererComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto& [sr, tf] = view.get<SpriteRendererComponent, TransformComponent>(entity);
				Ref<Texture2D> texture;
				if (ResourceManager::IsValidAssetHandle(sr.TextureHandle))
					texture = ResourceManager::GetAsset<Texture2D>(sr.TextureHandle);
				renderer->SubmitQuad(tf.Position, tf.Rotation, tf.Scaling, texture, sr.TilingFactor, sr.Color, (int)entity);
			}
		}

		{
			auto view = m_Registry.view<CircleRendererComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto& [cr, tf] = view.get<CircleRendererComponent, TransformComponent>(entity);
				renderer->SubmitCirlce(tf.Position, tf.Rotation, tf.Scaling, cr.Color, cr.Thickness, cr.Fade, (int)entity);
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
			auto view = m_Registry.view<SpriteRendererComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto& [sr, tf] = view.get<SpriteRendererComponent, TransformComponent>(entity);
				Ref<Texture2D> texture;
				if (ResourceManager::IsValidAssetHandle(sr.TextureHandle))
					texture = ResourceManager::GetAsset<Texture2D>(sr.TextureHandle);
				renderer->SubmitQuad(tf.Position, tf.Rotation, tf.Scaling, texture, sr.TilingFactor, sr.Color, (int)entity);
			}
		}

		{
			auto view = m_Registry.view<CircleRendererComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto& [cr, tf] = view.get<CircleRendererComponent, TransformComponent>(entity);
				renderer->SubmitCirlce(tf.Position, tf.Rotation, tf.Scaling, cr.Color, cr.Thickness, cr.Fade, (int)entity);
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
			auto view = m_Registry.view<SpriteRendererComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto& [sr, tf] = view.get<SpriteRendererComponent, TransformComponent>(entity);
				Ref<Texture2D> texture;
				if (ResourceManager::IsValidAssetHandle(sr.TextureHandle))
					texture = ResourceManager::GetAsset<Texture2D>(sr.TextureHandle);
				renderer->SubmitQuad(tf.Position, tf.Rotation, tf.Scaling, texture, sr.TilingFactor, sr.Color, (int)entity);
			}
		}

		{
			auto view = m_Registry.view<CircleRendererComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto& [cr, tf] = view.get<CircleRendererComponent, TransformComponent>(entity);
				renderer->SubmitCirlce(tf.Position, tf.Rotation, tf.Scaling, cr.Color, cr.Thickness, cr.Fade, (int)entity);
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

		Entity entity{ m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid ? uuid : UUID::Generate());
		entity.AddComponent<TagComponent>(tag.empty() ? "new Entity" : tag);
		entity.AddComponent<TransformComponent>();

		m_EntityUUIDMap[uuid] = entity;
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		SK_PROFILE_FUNCTION();
		
		if (!entity)
			return;

		if (!m_IsEditorScene && entity.HasComponent<ScriptComponent>())
			ScriptManager::Destroy(entity, true);

		m_EntityUUIDMap.erase(entity.GetUUID());
		m_Registry.destroy(entity);
	}

	Entity Scene::GetEntityByUUID(UUID uuid)
	{
		SK_PROFILE_FUNCTION();
		
		const auto entry = m_EntityUUIDMap.find(uuid);
		if (entry != m_EntityUUIDMap.end())
			return entry->second;
		return Entity{};
	}

	bool Scene::IsValidEntity(Entity entity)const
	{
		SK_PROFILE_FUNCTION();
		
		return m_Registry.valid(entity);
	}

	Entity Scene::FindActiveCameraEntity()
	{
		SK_PROFILE_FUNCTION();
		
		if (m_ActiveCameraUUID.IsValid())
			return m_EntityUUIDMap.at(m_ActiveCameraUUID);
		return Entity{};
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
				auto& transform = entity.GetTransform();

				b2BodyDef bodydef;
				bodydef.type = SharkBodyTypeToBox2D(rb2d.Type);
				bodydef.position = { transform.Position.x, transform.Position.y };
				bodydef.angle = transform.Rotation.z;
				bodydef.fixedRotation = rb2d.FixedRotation;
				bodydef.bullet = rb2d.IsBullet;
				bodydef.awake = rb2d.Awake;
				bodydef.enabled = rb2d.Enabled;
				bodydef.gravityScale = rb2d.GravityScale;
				bodydef.allowSleep = rb2d.AllowSleep;
				bodydef.userData.pointer = (uintptr_t)entity.GetUUID();

				rb2d.RuntimeBody = world->CreateBody(&bodydef);

				if (entity.HasComponent<BoxCollider2DComponent>())
				{
					auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

					b2PolygonShape shape;
					shape.SetAsBox(bc2d.Size.x * transform.Scaling.x, bc2d.Size.y * transform.Scaling.y, { bc2d.Offset.x, bc2d.Offset.y }, bc2d.Rotation);

					b2FixtureDef fixturedef;
					fixturedef.shape = &shape;
					fixturedef.friction = bc2d.Friction;
					fixturedef.density = bc2d.Density;
					fixturedef.restitution = bc2d.Restitution;
					fixturedef.restitutionThreshold = bc2d.RestitutionThreshold;
					fixturedef.isSensor = bc2d.IsSensor;

					bc2d.RuntimeCollider = rb2d.RuntimeBody->CreateFixture(&fixturedef);
				}

				if (entity.HasComponent<CircleCollider2DComponent>())
				{
					auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

					b2CircleShape shape;
					shape.m_radius = cc2d.Radius * transform.Scaling.x;
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

	void Scene::OnRigidBody2DComponentCreated(entt::registry& registry, entt::entity entityID)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ entityID, this };
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
		auto& transform = entity.GetTransform();

		b2BodyDef bodydef;
		bodydef.type = SharkBodyTypeToBox2D(rb2d.Type);
		bodydef.position = { transform.Position.x, transform.Position.y };
		bodydef.angle = transform.Rotation.z;
		bodydef.fixedRotation = rb2d.FixedRotation;
		bodydef.userData.pointer = (uintptr_t)entity.GetUUID();

		rb2d.RuntimeBody = m_PhysicsScene.GetWorld()->CreateBody(&bodydef);
	}

	void Scene::OnBoxCollider2DComponentCreated(entt::registry& registry, entt::entity entityID)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ entityID, this };
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
		auto& transform = entity.GetTransform();
		auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

		b2PolygonShape shape;
		shape.SetAsBox(bc2d.Size.x * transform.Scaling.x, bc2d.Size.y * transform.Scaling.y, { bc2d.Offset.x, bc2d.Offset.y }, bc2d.Rotation);

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
		auto& transform = entity.GetTransform();
		auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

		b2CircleShape shape;
		shape.m_radius = cc2d.Radius * transform.Scaling.x;
		shape.m_p = { cc2d.Offset.x, cc2d.Offset.y };

		b2FixtureDef fixturedef;
		fixturedef.shape = &shape;
		fixturedef.friction = cc2d.Friction;
		fixturedef.density = cc2d.Density;
		fixturedef.restitution = cc2d.Restitution;
		fixturedef.restitutionThreshold = cc2d.RestitutionThreshold;

		cc2d.RuntimeCollider = rb2d.RuntimeBody->CreateFixture(&fixturedef);
	}

	void Scene::OnRigidBody2DComponentDestroyed(entt::registry& registry, entt::entity entityID)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ entityID, this };
		auto& comp = entity.GetComponent<RigidBody2DComponent>();
		m_PhysicsScene.GetWorld()->DestroyBody(comp.RuntimeBody);
		comp.RuntimeBody = nullptr;
	}

	void Scene::OnBoxCollider2DComponentDestroyed(entt::registry& registry, entt::entity entityID)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ entityID, this };
		if (!entity.HasComponent<RigidBody2DComponent>())
			return;

		auto& comp = entity.GetComponent<BoxCollider2DComponent>();
		b2Body* body = comp.RuntimeCollider->GetBody();
		body->DestroyFixture(comp.RuntimeCollider);
		comp.RuntimeCollider = nullptr;
	}

	void Scene::OnCircleCollider2DComponentDestroyed(entt::registry& registry, entt::entity entityID)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ entityID, this };
		if (!entity.HasComponent<RigidBody2DComponent>())
			return;

		auto& comp = entity.GetComponent<CircleCollider2DComponent>();
		b2Body* body = comp.RuntimeCollider->GetBody();
		body->DestroyFixture(comp.RuntimeCollider);
		comp.RuntimeCollider = nullptr;
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
