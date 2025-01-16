#include "skpch.h"
#include "Physics2DScene.h"

#include "Shark/Core/Project.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

#include "Shark/Debug/Profiler.h"

#include <box2d/b2_contact.h>

namespace Shark {

	Physics2DScene::Physics2DScene(Ref<Scene> sceneContext, bool connectContactListener)
	{
		m_SceneContext = sceneContext;

		const auto& config = Project::GetActive()->Physics;
		m_Gravity = config.Gravity;
		m_VelocityIterations = config.VelocityIterations;
		m_PositionIterations = config.PositionIterations;
		m_FixedTimeStep = config.FixedTimeStep;
		m_MaxTimestep = config.MaxTimestep;

		m_World = sknew b2World({ m_Gravity.x, m_Gravity.y });

		if (connectContactListener)
		{
			m_ContactListener = sknew ContactListener([this](ContactType contactType, UUID entityA, UUID entityB)
			{
				OnContactEvent(contactType, entityA, entityB);
			});

			m_World->SetContactListener(m_ContactListener);
		}
	}

	Physics2DScene::~Physics2DScene()
	{
		skdelete m_World;
		skdelete m_ContactListener;
	}

	void Physics2DScene::Step(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Physics2DScene::Step");

		m_Profile.Reset();

		const float timeStep = std::min(ts.Seconds(), m_MaxTimestep);
		m_Profile.TimeStep = timeStep;

		m_Accumulator += timeStep;
		while (m_Accumulator >= m_FixedTimeStep)
		{
			if (m_OnPhysicsStep)
				m_OnPhysicsStep(m_FixedTimeStep);

			m_World->Step(m_FixedTimeStep, m_VelocityIterations, m_PositionIterations);
			m_Accumulator -= m_FixedTimeStep;

			m_Profile.AddStep(m_World->GetProfile());
		}
	}

	void Physics2DScene::DestroyAllBodies()
	{
		b2Body* iter = m_World->GetBodyList();
		while (iter)
		{
			b2Body* body = iter;
			iter = iter->GetNext();
			m_World->DestroyBody(body);
		}
	}

	b2Body* Physics2DScene::GetBody(Entity entity) const
	{
		if (entity && entity.HasComponent<RigidBody2DComponent>())
		{
			auto& comp = entity.GetComponent<RigidBody2DComponent>();
			return comp.RuntimeBody;
		}
		return nullptr;
	}

	void Physics2DScene::OnContactEvent(ContactType contactType, UUID entityA, UUID entityB)
	{
		const auto callScriptContact = [](std::string_view methodName, Entity entityA, Entity entityB)
		{
			if (!entityA.HasComponent<ScriptComponent>())
				return;

			const auto& scriptComponent = entityA.GetComponent<ScriptComponent>();
			if (scriptComponent.Instance)
				scriptComponent.Instance->InvokeMethod(methodName, (uint64_t)entityB.GetUUID());
		};

		Entity a = m_SceneContext->TryGetEntityByUUID(entityA);
		Entity b = m_SceneContext->TryGetEntityByUUID(entityB);

		switch (contactType)
		{
			case ContactType::CollisionBegin:
			{
				callScriptContact("InvokeCollishionBeginEvent", a, b);
				callScriptContact("InvokeCollishionBeginEvent", b, a);
				break;
			}
			case ContactType::CollisionEnd:
			{
				callScriptContact("InvokeCollishionEndEvent", a, b);
				callScriptContact("InvokeCollishionEndEvent", b, a);
				break;
			}
			case ContactType::TriggerBegin:
			{
				callScriptContact("InvokeTriggerBeginEvent", a, b);
				callScriptContact("InvokeTriggerBeginEvent", b, a);
				break;
			}
			case ContactType::TriggerEnd:
			{
				callScriptContact("InvokeTriggerEndEvent", a, b);
				callScriptContact("InvokeTriggerEndEvent", b, a);
				break;
			}
		}

	}

	void PhysicsProfile::Reset()
	{
		TimeStep = 0.0f;
		NumSteps = 0;
		Step = 0.0f;
		Collide = 0.0f;
		Solve = 0.0f;
		SolveInit = 0.0f;
		SolveVelocity = 0.0f;
		SolvePosition = 0.0f;
		Broadphase = 0.0f;
		SolveTOI = 0.0f;
	}

	void PhysicsProfile::AddStep(const b2Profile& p)
	{
		NumSteps++;
		Step += p.step * 0.001f;
		Collide += p.collide * 0.001f;
		Solve += p.solve * 0.001f;
		SolveInit += p.solveInit * 0.001f;
		SolveVelocity += p.solveVelocity * 0.001f;
		SolvePosition += p.solvePosition * 0.001f;
		Broadphase += p.broadphase * 0.001f;
		SolveTOI += p.solveTOI * 0.001f;
	}

	b2Vec2 Physics2DUtils::ToB2Vec(const glm::vec2& vec)
	{
		return { vec.x, vec.y };
	}

	glm::vec2 Physics2DUtils::FromBody(b2Body* body)
	{
		const b2Vec2& pos = body->GetPosition();
		return { pos.x, pos.y };
	}

	glm::mat4 Physics2DUtils::GetMatrix(b2Body* body)
	{
		return glm::translate(glm::vec3(FromBody(body), 0.0f)) *
			glm::toMat4(glm::quat(glm::vec3(0.0f, 0.0f, body->GetAngle())));
	}

	b2BodyType Physics2DUtils::ConvertBodyType(RigidbodyType type)
	{
		switch (type)
		{
			case RigidbodyType::Static: return b2_staticBody;
			case RigidbodyType::Dynamic: return b2_dynamicBody;
			case RigidbodyType::Kinematic: return b2_kinematicBody;
		}
		SK_CORE_VERIFY(false, "Unkown RigidbodyType");
		return b2_staticBody;
	}

	RigidbodyType Physics2DUtils::ConvertBodyType(b2BodyType type)
	{
		switch (type)
		{
			case b2_staticBody: return RigidbodyType::Static;
			case b2_kinematicBody: return RigidbodyType::Kinematic;
			case b2_dynamicBody: return RigidbodyType::Dynamic;
		}
		SK_CORE_VERIFY(false, "Unkown b2BodyType");
		return RigidbodyType::Static;
	}

}
