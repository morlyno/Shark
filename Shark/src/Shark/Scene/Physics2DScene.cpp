#include "skpch.h"
#include "Physics2DScene.h"

#include "Shark/Core/Project.h"

#include "Shark/Debug/Instrumentor.h"

#include <box2d/b2_contact.h>

namespace Shark {

	Physics2DScene::Physics2DScene()
		: m_Gravity(Project::Gravity()), m_VelocityIterations(Project::VelocityIterations()),
		  m_PositionIterations(Project::PositionIterations()), m_FixedTimeStep(Project::FixedTimeStep())
	{
		SK_PROFILE_FUNCTION();
	}

	Physics2DScene::~Physics2DScene()
	{
		SK_PROFILE_FUNCTION();

		if (m_World)
			DestoryScene();
	}

	void Physics2DScene::CreateScene()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(!m_World);
		m_World = new b2World({ m_Gravity.x, m_Gravity.y });
	}

	void Physics2DScene::DestoryScene()
	{
		SK_PROFILE_FUNCTION();

		if (m_World)
		{
			delete m_World;
			m_World = nullptr;
		}
	}

	void Physics2DScene::Step(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		m_Profile.Reset();

		const float timeStep = std::min(ts.Seconds(), 0.016f);
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
		Step += p.step;
		Collide += p.collide;
		Solve += p.solve;
		SolveInit += p.solveInit;
		SolveVelocity += p.solveVelocity;
		SolvePosition += p.solvePosition;
		Broadphase += p.broadphase;
		SolveTOI += p.solveTOI;
	}

	glm::vec2 Phyiscs2DUtils::FromBody(b2Body* body)
	{
		const b2Vec2& pos = body->GetPosition();
		return { pos.x, pos.y };
	}

	glm::mat4 Phyiscs2DUtils::GetMatrix(b2Body* body)
	{
		return glm::translate(glm::vec3(FromBody(body), 0.0f)) *
			glm::toMat4(glm::quat(glm::vec3(body->GetAngle())));
	}

}
