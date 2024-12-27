#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/TimeStep.h"

#include "Shark/Physics2D/ContactListener.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>

namespace Shark {

	class Entity;
	class Scene;

	enum class Collider2DType
	{
		BoxCollider,
		CircleCollider
	};

	enum class RigidbodyType
	{
		Static,
		Dynamic,
		Kinematic
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
		Physics2DScene(Ref<Scene> sceneContext, bool connectContactListener = true);
		~Physics2DScene();

		void Step(TimeStep ts);

		void DestroyAllBodies();

		bool Active() const { return m_World != nullptr; }
		const PhysicsProfile& GetProfile() const { return m_Profile; }

		template<typename Func>
		void SetOnPhyicsStepCallback(const Func& func) { m_OnPhysicsStep = func; }

		b2World* GetWorld() const { return m_World; }
		b2Body* GetBody(Entity entity) const;

		float GetTimeStep() const { return m_FixedTimeStep; }

	private:
		void OnContactEvent(ContactType contactType, UUID entityA, UUID entityB);

	private:
		Ref<Scene> m_SceneContext;
		b2World* m_World = nullptr;
		ContactListener* m_ContactListener;

		glm::vec2 m_Gravity = { 0.0f, -9.81f };
		uint32_t m_VelocityIterations = 8;
		uint32_t m_PositionIterations = 3;

		float m_Accumulator = 0.0f;
		float m_FixedTimeStep = 0.001f;
		float m_MaxTimestep = 0.016f;

		std::function<void(TimeStep)> m_OnPhysicsStep;

		PhysicsProfile m_Profile;
	};

	namespace Physics2DUtils {

		b2Vec2 ToB2Vec(const glm::vec2& vec);
		inline glm::vec2 ToGLMVec(const b2Vec2& vec) { return glm::vec2(vec.x, vec.y); }
		glm::vec2 FromBody(b2Body* body);
		glm::mat4 GetMatrix(b2Body* body);

		b2BodyType ConvertBodyType(RigidbodyType type);
		RigidbodyType ConvertBodyType(b2BodyType type);

	}

}
