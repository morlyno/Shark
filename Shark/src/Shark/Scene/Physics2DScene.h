#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/TimeStep.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>

namespace Shark {

	class Entity;

	enum class Collider2DType
	{
		BoxCollider,
		CircleCollider
	};

	struct PhysicsProfile
	{
		float TimeStep;
		uint32_t NumSteps;
		float Step;
		float Collide;
		float Solve;
		float SolveInit;
		float SolveVelocity;
		float SolvePosition;
		float Broadphase;
		float SolveTOI;

		void Reset();
		void AddStep(const b2Profile& p);
	};

	class Physics2DScene
	{
	public:
		Physics2DScene();
		~Physics2DScene();

		void CreateScene();
		void DestoryScene();

		void Step(TimeStep ts);

		void DestroyAllBodies();

		bool Active() const { return m_World != nullptr; }
		const PhysicsProfile& GetProfile() const { return m_Profile; }

		template<typename Func>
		void SetOnPhyicsStepCallback(const Func& func) { m_OnPhysicsStep = func; }

		b2World* GetWorld() const { return m_World; }
	private:
		b2World* m_World = nullptr;

		glm::vec2 m_Gravity = { 0.0f, -9.81f };
		uint32_t m_VelocityIterations = 8;
		uint32_t m_PositionIterations = 3;

		float m_Accumulator = 0.0f;
		float m_FixedTimeStep = 0.001f;

		std::function<void(TimeStep)> m_OnPhysicsStep;

		PhysicsProfile m_Profile;
	};

	namespace Phyiscs2DUtils {

		glm::vec2 FromBody(b2Body* body);
		glm::mat4 GetMatrix(b2Body* body);

	}

}
