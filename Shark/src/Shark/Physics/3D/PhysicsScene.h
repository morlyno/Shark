#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace Shark {

	namespace Layers {
		static constexpr JPH::ObjectLayer NonMoving = 0;
		static constexpr JPH::ObjectLayer Moving = 1;
		static constexpr JPH::ObjectLayer Count = 2;
	}

	namespace BroadPhaseLayers {
		static constexpr JPH::BroadPhaseLayer NonMoving{ 0 };
		static constexpr JPH::BroadPhaseLayer Moving{ 1 };
	}

	class BroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
	{
	public:
		BroadPhaseLayerInterface();
		virtual JPH::uint GetNumBroadPhaseLayers() const override;
		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif

	private:
		JPH::BroadPhaseLayer m_ObjectToBroadPhase[Layers::Count];
	};

	class ObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide([[maybe_unused]] JPH::ObjectLayer inLayer1, [[maybe_unused]] JPH::BroadPhaseLayer inLayer2) const;
	};

	class ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///// Physics System ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class PhysicsScene : public RefCount
	{
	public:
		PhysicsScene(Ref<Scene> scene);
		~PhysicsScene();

		void Update(TimeStep timeStep);

		JPH::Body* CreateBody(Entity entity);
		
	private:
		JPH::Ref<JPH::Shape> CreateShape(Entity entity, const TransformComponent& worldTransform);

	private:
		Ref<Scene> m_Scene;
		JPH::PhysicsSystem m_System;

		BroadPhaseLayerInterface m_BroadPhaseLayerInterface;
		ObjectVsBroadPhaseLayerFilter m_ObjectVsBroadPhaseLayerFilter;
		ObjectLayerPairFilter m_ObjectLayerPairFilter;

	};

}
