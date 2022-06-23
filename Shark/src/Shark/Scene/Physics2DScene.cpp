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

		// TODO(moro): cap ts
		m_Accumulator += ts;

		while (m_Accumulator >= m_FixedTimeStep)
		{
			m_World->Step(m_FixedTimeStep, m_VelocityIterations, m_PositionIterations);
			m_Accumulator -= m_FixedTimeStep;
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

	bool Physics2DScene::HasBody(const b2Body* body) const
	{
		SK_PROFILE_FUNCTION();

		const b2Body* iter = m_World->GetBodyList();

		while (iter)
		{
			if (iter == body)
				return true;
			iter = iter->GetNext();
		}
		return false;
	}

	bool Physics2DScene::HodyHasCollider(const b2Body* body, const b2Fixture* fixture)
	{
		SK_PROFILE_FUNCTION();

		const b2Fixture* iter = body->GetFixtureList();

		while (iter)
		{
			if (iter == fixture)
				return true;
			iter = iter->GetNext();
		}
		return false;
	}

}
