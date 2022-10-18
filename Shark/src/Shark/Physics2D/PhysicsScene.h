#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

#include "Shark/Physics2D/Physics2DActor.h"
#include "Shark/Physics2D/ContactListener.h"

class b2World;

namespace Shark {

	class Scene;
	class Entity;

	enum class Collider2DType
	{
		None = 0,
		BoxCollider,
		CircleCollider
	};

	class Physics2DScene : public RefCount
	{
	public:
		Physics2DScene(Ref<Scene> scene);
		~Physics2DScene();

		void SetSceneContext(Ref<Scene> scene);
		Ref<Scene> SetSceneContext() const;

		void Simulate(TimeStep ts);

	public:
		Ref<Physics2DActor> CreateActor(Entity entity);
		void DestroyActor(Entity entity);

		Ref<Physics2DActor> GetActor(Entity entity) const;
		bool HasActor(Entity entity) const;

		void OnColliderCreated(Entity entity, BoxCollider2DComponent& collider);
		void OnColliderCreated(Entity entity, CircleCollider2DComponent& collider);
		void OnColliderDestroyed(Entity entity, BoxCollider2DComponent& collider);
		void OnColliderDestroyed(Entity entity, CircleCollider2DComponent& collider);

	public:
		const glm::vec2& GetGravity() const { return m_Gravity; }
		void SetGravity(const glm::vec2& gravity);

		uint32_t GetColocityIterations() const { return m_VelocityIterations; }
		void SetColocityIterations(uint32_t iterations) { m_VelocityIterations = iterations; }

		uint32_t GetPositionIterations() const { return m_PositionIterations; }
		void SetPositionIterations(uint32_t iterations) { m_PositionIterations = iterations; }

		float GetFixedTimeStep() const { return m_FixedTimeStep; }
		void SetFixedTimeStep(float fixedTimeStep) { m_FixedTimeStep = fixedTimeStep; }

	private:
		bool Advance(float ts);
		void CalcSubsteps(float ts);

		void CreateRigidBodies();
		bool ShouldBeStaticActor(Entity entity);

	private:
		Ref<Scene> m_EntityScene;
		b2World* m_World = nullptr;
		ContactListener m_Listener;

		glm::vec2 m_Gravity = { 0.0f, -9.81f };
		uint32_t m_VelocityIterations = 8;
		uint32_t m_PositionIterations = 3;

		uint32_t m_SubSteps = 0;
		float m_Accumulator = 0.0f;
		float m_FixedTimeStep = 0.001f;

		std::unordered_map<UUID, Ref<Physics2DActor>> m_Actors;

	};

}
