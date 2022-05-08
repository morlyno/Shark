#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/TimeStep.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>

namespace Shark {

	class Entity;

	class Physics2DScene
	{
	public:
		Physics2DScene();
		~Physics2DScene();

		void CreateScene();
		void DestoryScene();

		void Step(TimeStep ts);

		bool HasBody(const b2Body* body) const;
		bool HodyHasCollider(const b2Body* body, const b2Fixture* fixture);

		b2World* GetWorld() const { return m_World; }
	private:
		b2World* m_World = nullptr;

		glm::vec2 m_Gravity = { 0.0f, -9.81f };
		uint32_t m_VelocityIterations = 8;
		uint32_t m_PositionIterations = 3;

		float m_Accumulator = 0.0f;
		float m_FixedTimeStep = 0.001f;
	};

}
