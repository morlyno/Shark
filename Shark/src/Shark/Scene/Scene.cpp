#include "skpch.h"
#include "Scene.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"
#include "Shark/Render/Renderer.h"

#include "Shark/Debug/Instrumentor.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_fixture.h>

#include "Shark/Debug/DebugEntity.h"

namespace Shark {

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

		SK_CORE_ASSERT(m_PhysicsWorld2D == nullptr, "OnSceneStop musst be called befor the Scene gets destroyed!");
	}

	Ref<Scene> Scene::GetCopy()
	{
		SK_PROFILE_FUNCTION();

		auto scene = Ref<Scene>::Create();
		CopyInto(scene);
		return scene;
	}

	void Scene::CopyInto(Ref<Scene> dest)
	{
		SK_PROFILE_FUNCTION();

		dest->m_Registry = entt::registry{};
		dest->m_Registry.reserve(m_Registry.capacity());
		m_Registry.each([this, dest](auto entity)
		{
			dest->CopyEntity({ entity, this });
		});

		dest->m_ActiveCameraID = m_ActiveCameraID;
		dest->m_ViewportWidth = m_ViewportWidth;
		dest->m_ViewportHeight = m_ViewportHeight;
		dest->m_FilePath = m_FilePath;
	}

	void Scene::OnUpdateRuntime(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

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
			m_PhysicsWorld2D->Step(ts, 6, 2);

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

		if (!m_Registry.valid(m_ActiveCameraID) || !m_Registry.has<CameraComponent, TransformComponent>(m_ActiveCameraID))
		{
			SK_CORE_WARN("Active Camera Entity is Invalid");
			auto view = m_Registry.view<CameraComponent>();
			m_ActiveCameraID = view[0];
		}
		auto&& [camera, transform] = m_Registry.get<CameraComponent, TransformComponent>(m_ActiveCameraID);

		{
			SK_PROFILE_SCOPE("Render Scene Runtime");

			Renderer2D::BeginScene(camera.Camera, DirectX::XMMatrixInverse(nullptr, transform.GetTranform()));
			{
				auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
				for (auto entityID : group)
					Renderer2D::DrawEntity({ entityID, this });
			}
			Renderer2D::EndScene();
		}
	}

	void Scene::OnUpdateEditor(TimeStep ts, EditorCamera& camera)
	{
		SK_PROFILE_FUNCTION();
		{
			SK_PROFILE_SCOPE("Render Scene Editor");

			Renderer2D::BeginScene(camera);
			{
				auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
				for (auto entityID : group)
					Renderer2D::DrawEntity({ entityID, this });
			}
			Renderer2D::EndScene();
		}
	}

	void Scene::OnScenePlay()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(m_PhysicsWorld2D == nullptr);
		m_PhysicsWorld2D = new b2World({ 0.0f, -9.8f });

		{
			auto view = m_Registry.view<BoxCollider2DComponent>();
			for (auto entity : view)
				if (!m_Registry.has<RigidBody2DComponent>(entity))
					m_Registry.emplace<RigidBody2DComponent>(entity);
		}

		{
			auto view = m_Registry.view<RigidBody2DComponent>();
			for (auto entity : view)
			{
				auto& rb2d = view.get<RigidBody2DComponent>(entity);
				auto& transform = m_Registry.get<TransformComponent>(entity);

				b2BodyDef bodydef;
				bodydef.type = SharkBodyTypeToBox2D(rb2d.Type);
				bodydef.position = { transform.Position.x, transform.Position.y };
				bodydef.angle = transform.Rotation.z;
				bodydef.fixedRotation = rb2d.FixedRotation;

				rb2d.RuntimeBody = m_PhysicsWorld2D->CreateBody(&bodydef);

				if (m_Registry.has<BoxCollider2DComponent>(entity))
				{
					auto& bc2d = m_Registry.get<BoxCollider2DComponent>(entity);

					b2PolygonShape shape;
					shape.SetAsBox(bc2d.Size.x * transform.Scaling.x, bc2d.Size.y * transform.Scaling.y, { bc2d.LocalOffset.x, bc2d.LocalOffset.y }, bc2d.LocalRotation);

					b2FixtureDef fixturedef;
					fixturedef.shape = &shape;
					fixturedef.friction = bc2d.Friction;
					fixturedef.density = bc2d.Density;
					fixturedef.restitution = bc2d.Restitution;
					fixturedef.restitutionThreshold = bc2d.RestitutionThreshold;

					bc2d.RuntimeCollider = rb2d.RuntimeBody->CreateFixture(&fixturedef);
				}
			}
		}


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
	}

	Entity Scene::CopyEntity(Entity srcEntity)
	{
		SK_PROFILE_FUNCTION();

		Entity newEntity = CreateEntity(srcEntity);

		CopyComponentIfExists<TagComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<TransformComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<SpriteRendererComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<CameraComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<NativeScriptComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<RigidBody2DComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<BoxCollider2DComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);

		return newEntity;
	}

	Entity Scene::CreateEntity(const std::string& tag)
	{
		SK_PROFILE_FUNCTION();

		Entity entity{ m_Registry.create(), this };
		auto& tagcomp = entity.AddComponent<TagComponent>();
		tagcomp.Tag = tag.empty() ? "new Entity" : tag;
		entity.AddComponent<TransformComponent>();
		return entity;
	}

	Entity Scene::CreateEntity(entt::entity hint, const std::string& tag)
	{
		SK_PROFILE_FUNCTION();

		Entity entity{ m_Registry.create(hint), this };
		auto& tagcomp = entity.AddComponent<TagComponent>();
		tagcomp.Tag = tag.empty() ? "new Entity" : tag;
		entity.AddComponent<TransformComponent>();
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		SK_PROFILE_FUNCTION();

		m_Registry.destroy(entity);
	}

	bool Scene::IsValidEntity(Entity entity)const
	{
		SK_PROFILE_FUNCTION();

		return m_Registry.valid(entity);
	}

	Entity Scene::GetActiveCamera()
	{
		return { m_ActiveCameraID, Weak(this) };
	}

	void Scene::SetActiveCamera(Entity cameraentity)
	{
		m_ActiveCameraID = cameraentity;
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

}
