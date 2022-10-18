#include "skpch.h"
#include "PhysicsScene.h"

#include "Shark/Core/Project.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

#include "Shark/Physics2D/Physics2DUtils.h"

#include "Shark/Math/Math.h"
#include "Shark/Debug/enttDebug.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>

namespace Shark {

	namespace utils {

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

	}

	Physics2DScene::Physics2DScene(Ref<Scene> scene)
	{
		m_EntityScene = scene;

		auto project = Project::GetActive();
		m_Gravity = project->Gravity;
		m_VelocityIterations = project->VelocityIterations;
		m_PositionIterations = project->PositionIterations;
		m_FixedTimeStep = project->FixedTimeStep;

		m_Listener.SetContext(scene);

		m_World = new b2World({ m_Gravity.x, m_Gravity.y });
		m_World->SetContactListener(&m_Listener);
		m_World->SetAutoClearForces(false);

		CreateRigidBodies();
	}

	Physics2DScene::~Physics2DScene()
	{
		m_Listener.SetContext(nullptr);
		delete m_World;
		m_World = nullptr;
		m_EntityScene = nullptr;
		m_Actors.clear();
	}

	void Physics2DScene::SetSceneContext(Ref<Scene> scene)
	{
		m_EntityScene = scene;
	}

	Ref<Scene> Physics2DScene::SetSceneContext() const
	{
		return m_EntityScene;
	}

	void Physics2DScene::Simulate(TimeStep ts)
	{
		if (Advance(ts))
		{
			for (auto& [id, actor] : m_Actors)
			{
				if (actor->IsStatic())
					continue;

				actor->SyncronizeTransforms();
			}
		}
	}

	Ref<Physics2DActor> Physics2DScene::CreateActor(Entity entity)
	{
		SK_CORE_ASSERT(!HasActor(entity));
		SK_CORE_ASSERT(entity.AllOf<RigidBody2DComponent>(), "Tried to create Actor without RigiBody2DComponent")
		if (!entity.AllOf<RigidBody2DComponent>())
			return nullptr;

		auto transform = m_EntityScene->GetWorldSpaceTransform(entity);
		auto& comp = entity.GetComponent<RigidBody2DComponent>();

		auto actor = Ref<Physics2DActor>::Create();
		m_Actors[entity.GetUUID()] = actor;

		b2BodyDef bodydef;
		bodydef.type = utils::SharkBodyTypeToBox2D(comp.Type);
		bodydef.position = { transform.Translation.x, transform.Translation.y };
		bodydef.angle = transform.Rotation.z;
		bodydef.fixedRotation = comp.FixedRotation;
		bodydef.bullet = comp.IsBullet;
		bodydef.awake = comp.Awake;
		bodydef.enabled = comp.Enabled;
		bodydef.gravityScale = comp.GravityScale;
		bodydef.allowSleep = comp.AllowSleep;
		bodydef.userData.pointer = (uintptr_t)actor.Raw();
		b2Body* body = m_World->CreateBody(&bodydef);
		actor->m_Entity = entity;
		actor->m_Body = body;

		if (entity.AllOf<BoxCollider2DComponent>()) actor->AddCollider(entity.GetComponent<BoxCollider2DComponent>());
		if (entity.AllOf<CircleCollider2DComponent>()) actor->AddCollider(entity.GetComponent<CircleCollider2DComponent>());

		return actor;
	}

	void Physics2DScene::DestroyActor(Entity entity)
	{
		if (!HasActor(entity))
			return;

		auto actor = GetActor(entity);
		if (entity.AllOf<BoxCollider2DComponent>()) actor->RemoveCollider(entity.GetComponent<BoxCollider2DComponent>());
		if (entity.AllOf<CircleCollider2DComponent>()) actor->RemoveCollider(entity.GetComponent<CircleCollider2DComponent>());

		m_Actors.erase(actor->GetEntity().GetUUID());
		m_World->DestroyBody(actor->GetBody());
		actor->m_Body = nullptr;
		actor->m_Entity = Entity{};
	}

	Ref<Physics2DActor> Physics2DScene::GetActor(Entity entity) const
	{
		return m_Actors.at(entity.GetUUID());
	}

	bool Physics2DScene::HasActor(Entity entity) const
	{
		return m_Actors.find(entity.GetUUID()) != m_Actors.end();
	}

	void Physics2DScene::OnColliderCreated(Entity entity, BoxCollider2DComponent& collider)
	{
		// TODO(moro): compound component
		if (!entity.AllOf<RigidBody2DComponent>())
			return;

		if (HasActor(entity))
		{
			auto actor = GetActor(entity);
			actor->AddCollider(collider);
		}
	}

	void Physics2DScene::OnColliderCreated(Entity entity, CircleCollider2DComponent& collider)
	{
		// TODO(moro): compound component
		if (!entity.AllOf<RigidBody2DComponent>())
			return;

		if (HasActor(entity))
		{
			auto actor = GetActor(entity);
			actor->AddCollider(collider);
		}
	}

	void Physics2DScene::OnColliderDestroyed(Entity entity, BoxCollider2DComponent& collider)
	{
		// TODO(moro): compound component
		if (!entity.AllOf<RigidBody2DComponent>())
			return;

		if (HasActor(entity))
		{
			auto actor = GetActor(entity);
			actor->RemoveCollider(collider);
		}
	}

	void Physics2DScene::OnColliderDestroyed(Entity entity, CircleCollider2DComponent& collider)
	{
		// TODO(moro): compound component
		if (!entity.AllOf<RigidBody2DComponent>())
			return;

		if (HasActor(entity))
		{
			auto actor = GetActor(entity);
			actor->RemoveCollider(collider);
		}
	}

	void Physics2DScene::SetGravity(const glm::vec2& gravity)
	{
		m_Gravity = gravity;
		m_World->SetGravity({ gravity.x, gravity.y });
	}

	bool Physics2DScene::Advance(float ts)
	{
		CalcSubsteps(ts);

		for (uint32_t i = 0; i < m_SubSteps; i++)
		{
			m_EntityScene->OnFixedUpdate(m_FixedTimeStep);
			m_World->Step(m_FixedTimeStep, m_VelocityIterations, m_PositionIterations);
			m_World->ClearForces();
		}
		
		return m_SubSteps != 0;
	}

	void Physics2DScene::CalcSubsteps(float ts)
	{
		m_Accumulator += ts;
		if (m_Accumulator < m_FixedTimeStep)
		{
			m_SubSteps = 0;
			return;
		}

		m_SubSteps = (uint32_t)(m_Accumulator / m_FixedTimeStep);
		m_Accumulator -= m_SubSteps * m_FixedTimeStep;
	}

	void Physics2DScene::CreateRigidBodies()
	{
		{
			auto view = m_EntityScene->GetAllEntitiesWith<BoxCollider2DComponent>(entt::exclude<RigidBody2DComponent>);
			for (auto ent : view)
			{
				Entity entity = { ent, m_EntityScene };
				if (ShouldBeStaticActor(entity))
				{
					SK_CORE_TRACE_TAG("Physics2D", "Added RigidBody to Entity with only Box Collider ({0})", entity.GetName());
					auto& comp = entity.AddComponent<RigidBody2DComponent>();
					comp.Type = RigidBody2DComponent::BodyType::Static;
				}
			}
		}

		{
			auto view = m_EntityScene->GetAllEntitiesWith<CircleCollider2DComponent>(entt::exclude<RigidBody2DComponent>);
			for (auto ent : view)
			{
				Entity entity = { ent, m_EntityScene };
				if (ShouldBeStaticActor(entity))
				{
					SK_CORE_TRACE_TAG("Physics2D", "Added RigidBody to Entity with only Circle Collider ({0})", entity.GetName());
					auto& comp = entity.AddComponent<RigidBody2DComponent>();
					comp.Type = RigidBody2DComponent::BodyType::Static;
				}
			}
		}
		
		{
			auto view = m_EntityScene->GetAllEntitiesWith<RigidBody2DComponent>();
			for (auto ent : view)
			{
				Entity entity = { ent, m_EntityScene };
				Debug::EntityView debugEntity = { entity };
				CreateActor(entity);
			}
		}
	}

	bool Physics2DScene::ShouldBeStaticActor(Entity entity)
	{
		// TODO(moro): returns false when included in compound collider
		return true;
	}

}
