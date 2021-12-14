#include "skpch.h"
#include "Scene.h"

#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"
#include "Shark/Asset/ResourceManager.h"
#include "Shark/Render/SceneRenderer.h"

#include "Shark/Utility/Math.h"

#include "Shark/Debug/enttDebug.h"
#include "Shark/Debug/Instrumentor.h"
#include "Shark/Debug/Profiler.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>
#include <box2d/b2_fixture.h>

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

		//SK_CORE_ASSERT(m_PhysicsWorld2D == nullptr, "OnSceneStop musst be called befor the Scene gets destroyed!");
		if (m_PhysicsWorld2D)
			OnSceneStop();
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

		return newScene;
	}

	void Scene::OnScenePlay()
	{
		SK_PROFILE_FUNCTION();
		
		SetupBox2D();

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
		ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight);
	}

	void Scene::OnSceneStop()
	{
		SK_PROFILE_FUNCTION();
		
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

		delete m_PhysicsWorld2D;
		m_PhysicsWorld2D = nullptr;

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
			auto view = m_Registry.view<NativeScriptComponent>();
			for (auto entityID : view)
			{
				auto& nsc = view.get<NativeScriptComponent>(entityID);
				if (nsc.Script)
					nsc.Script->OnUpdate(ts);
			}
		}

		{
			// TODO(moro): expose iteration values
			m_PhysicsWorld2D->Step((float)ts, 6, 2);

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

	void Scene::OnSimulate(TimeStep ts, bool subStep)
	{
		SK_PROFILE_FUNCTION();
		
		// TODO(moro): expose iteration values
		if (subStep)
		{
			m_PhysicsWorld2D->SetSubStepping(true);
			m_PhysicsWorld2D->Step((float)ts, 6, 2);
			m_PhysicsWorld2D->SetSubStepping(false);
		}
		else
			m_PhysicsWorld2D->Step((float)ts, 6, 2);

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

	void Scene::OnRenderRuntimePreview(Ref<SceneRenderer> renderer, const Camera& camera, const DirectX::XMMATRIX& view)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Scene::OnRenderRuntimePreview");

		renderer->SetScene(this);
		const auto viewProj = view * camera.GetProjection();
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

		const auto viewProj = DirectX::XMMatrixInverse(nullptr, tf.GetTranform()) * camera.GetProjection();
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


		if (renderer->GetOptions().ShowColliders)
		{
			{
				auto view = m_Registry.view<BoxCollider2DComponent, TransformComponent>();
				for (auto entity : view)
				{
					auto& [bc, tf] = view.get<BoxCollider2DComponent, TransformComponent>(entity);
					renderer->SubmitColliderBox({ tf.Position.x + bc.Offset.x, tf.Position.y + bc.Offset.y }, tf.Rotation.z + bc.Rotation, { tf.Scaling.x * bc.Size.x * 2.0f, tf.Scaling.y * bc.Size.y * 2.0f });
				}
			}

			{
				auto view = m_Registry.view<CircleCollider2DComponent, TransformComponent>();
				for (auto entity : view)
				{
					auto&& [cc, tf] = view.get<CircleCollider2DComponent, TransformComponent>(entity);
					renderer->SubmitColliderCirlce({ tf.Position.x + cc.Offset.x, tf.Position.y + cc.Offset.y }, tf.Scaling.x * cc.Radius);
				}
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

		if (renderer->GetOptions().ShowColliders)
		{
			{
				auto view = m_Registry.view<BoxCollider2DComponent, TransformComponent>();
				for (auto entity : view)
				{
					auto& [bc, tf] = view.get<BoxCollider2DComponent, TransformComponent>(entity);
					renderer->SubmitColliderBox({ tf.Position.x + bc.Offset.x, tf.Position.y + bc.Offset.y }, tf.Rotation.z + bc.Rotation, { tf.Scaling.x * bc.Size.x * 2.0f, tf.Scaling.y * bc.Size.y * 2.0f });
				}
			}

			{
				auto view = m_Registry.view<CircleCollider2DComponent, TransformComponent>();
				for (auto entity : view)
				{
					auto&& [cc, tf] = view.get<CircleCollider2DComponent, TransformComponent>(entity);
					renderer->SubmitColliderCirlce({ tf.Position.x + cc.Offset.x, tf.Position.y + cc.Offset.y }, tf.Scaling.x * cc.Radius);
				}
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
		
		Entity entity{ m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TagComponent>(tag.empty() ? "new Entity" : tag);
		entity.AddComponent<TransformComponent>();
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		SK_PROFILE_FUNCTION();
		
		m_Registry.destroy(entity);
	}

	Entity Scene::GetEntityByUUID(UUID uuid)
	{
		SK_PROFILE_FUNCTION();
		
		auto view = m_Registry.view<IDComponent>();
		for (auto e : view)
		{
			Entity entity{ e, this };
			if (entity.GetUUID() == uuid)
				return entity;
		}
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
		{
			auto view = m_Registry.view<CameraComponent>();
			for (auto e : view)
			{
				Entity entity{ e, this };
				if (entity.GetUUID() == m_ActiveCameraUUID)
					return entity;
			}
		}
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
		
		SK_CORE_ASSERT(m_PhysicsWorld2D == nullptr);
		m_PhysicsWorld2D = new b2World({ 0.0f, -9.8f });

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

				rb2d.RuntimeBody = m_PhysicsWorld2D->CreateBody(&bodydef);

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

					bc2d.RuntimeCollider = rb2d.RuntimeBody->CreateFixture(&fixturedef);
				}

				if (entity.HasComponent<CircleCollider2DComponent>())
				{
					auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

					b2CircleShape shape;
					shape.m_radius = cc2d.Radius * transform.Scaling.x;

					b2FixtureDef fixturedef;
					fixturedef.shape = &shape;
					fixturedef.friction = cc2d.Friction;
					fixturedef.density = cc2d.Density;
					fixturedef.restitution = cc2d.Restitution;
					fixturedef.restitutionThreshold = cc2d.RestitutionThreshold;

					cc2d.RuntimeCollider = rb2d.RuntimeBody->CreateFixture(&fixturedef);
				}
			}
		}
	}

}
