#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace Shark {

	class BroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface
	{
	public:
		virtual JPH::uint GetNumBroadPhaseLayers() const override { return 1; }
		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override { return JPH::BroadPhaseLayer{ 0 }; }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif
	};

	class ObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		virtual bool ShouldCollide([[maybe_unused]] JPH::ObjectLayer inLayer1, [[maybe_unused]] JPH::BroadPhaseLayer inLayer2) const { return true; }
	};

	class ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
	{
	public:
		virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override { return true; }
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

		void CreateBody(Entity entity);
		void DestroyBody(Entity entity);

	private:
		Ref<Scene> m_Scene;
		JPH::PhysicsSystem m_System;

		std::unordered_map<UUID, JPH::BodyID> m_RigidBodyIDs;

		BroadPhaseLayerInterface m_BroadPhaseLayerInterface;
		ObjectVsBroadPhaseLayerFilter m_ObjectVsBroadPhaseLayerFilter;
		ObjectLayerPairFilter m_ObjectLayerPairFilter;

	};

}
