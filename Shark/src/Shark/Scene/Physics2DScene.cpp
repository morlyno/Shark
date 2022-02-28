#include "skpch.h"
#include "Physics2DScene.h"

#include "Shark/Core/Project.h"

namespace Shark {

	Physics2DScene::Physics2DScene()
		: m_Gravity(Project::GetGravity()), m_VelocityIterations(Project::GetVelocityIterations()),
		  m_PositionIterations(Project::GetPositionIterations()), m_FixedTimeStep(Project::GetFixedTimeStep())
	{
	}

	Physics2DScene::~Physics2DScene()
	{
		if (m_World)
			DestoryScene();
	}

	void Physics2DScene::CreateScene()
	{
		SK_CORE_ASSERT(!m_World);
		m_World = new b2World({ m_Gravity.x, m_Gravity.y });
	}

	void Physics2DScene::DestoryScene()
	{
		if (m_World)
		{
			delete m_World;
			m_World = nullptr;
		}
	}

	void Physics2DScene::Step(TimeStep ts)
	{
		// TODO(moro): cap ts
		m_Accumulator += ts;

		while (m_Accumulator >= m_FixedTimeStep)
		{
			m_World->Step(m_FixedTimeStep, m_VelocityIterations, m_PositionIterations);
			m_Accumulator -= m_FixedTimeStep;
		}

	}

}
