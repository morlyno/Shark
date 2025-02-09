#include "skpch.h"
#include "PhysicsScene.h"

#include "Shark/Physics/3D/PhysicsSystem.h"
#include "Shark/Physics/3D/Utilities.h"
#include "Shark/Debug/Profiler.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>

namespace Shark {

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///// Broad Phase Layer Interface ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	BroadPhaseLayerInterface::BroadPhaseLayerInterface()
	{
		m_ObjectToBroadPhase[Layers::NonMoving] = BroadPhaseLayers::NonMoving;
		m_ObjectToBroadPhase[Layers::Moving] = BroadPhaseLayers::Moving;
	}

	JPH::uint BroadPhaseLayerInterface::GetNumBroadPhaseLayers() const
	{
		return Layers::Count;
	}

	JPH::BroadPhaseLayer BroadPhaseLayerInterface::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
	{
		JPH_ASSERT(inLayer < Layers::Count);
		return m_ObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	const char* BroadPhaseLayerInterface::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
	{
		switch ((JPH::BroadPhaseLayer::Type)inLayer)
		{
			case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NonMoving: return "NonMoving";
			case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::Moving: return "Moving";
			default: JPH_ASSERT(false); return "Invalid";
		}
	}
#endif

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///// Object Vs Broad Phase Layer Filter ////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool ObjectVsBroadPhaseLayerFilter::ShouldCollide([[maybe_unused]] JPH::ObjectLayer inLayer1, [[maybe_unused]] JPH::BroadPhaseLayer inLayer2) const
	{
		switch (inLayer1)
		{
			case Layers::NonMoving: return inLayer2 == BroadPhaseLayers::Moving;
			case Layers::Moving: return true;
			default: JPH_ASSERT(false); return false;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///// Object Layer Pair Filter //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool ObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const
	{
		switch (inLayer1)
		{
			case Layers::NonMoving: return inLayer2 == Layers::Moving;
			case Layers::Moving: return true;
			default: JPH_ASSERT(false); return false;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///// Physics System ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PhysicsScene::PhysicsScene(Ref<Scene> scene)
		: m_Scene(scene)
	{
		m_System.Init(65536, 0, 65536, 10240, m_BroadPhaseLayerInterface, m_ObjectVsBroadPhaseLayerFilter, m_ObjectLayerPairFilter);

	}

	PhysicsScene::~PhysicsScene()
	{
	}

	void PhysicsScene::Update(TimeStep timeStep)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Physics Update");

		auto* allocator = PhysicsSystem::GetAllocator();
		auto* jobSystem = PhysicsSystem::GetJobSystem();

		m_System.Update(timeStep, 1, allocator, jobSystem);

		{
			const auto& bodyLockInterface = m_System.GetBodyLockInterface();
			JPH::BodyIDVector activeBodies;
			m_System.GetActiveBodies(JPH::EBodyType::RigidBody, activeBodies);
			auto bodiesLock = JPH::BodyLockMultiRead(bodyLockInterface, activeBodies.data(), activeBodies.size());
			for (uint32_t i = 0; i < activeBodies.size(); i++)
			{
				const JPH::Body* body = bodiesLock.GetBody(i);
				if (!body)
					continue;
				
				Entity entity = m_Scene->TryGetEntityByUUID(body->GetUserData());
				if (!entity)
					continue;

				TransformComponent bodyTransform;
				bodyTransform.Translation = JoltUtils::ToGLM(body->GetPosition());
				bodyTransform.Rotation = JoltUtils::ToGLM(body->GetRotation().GetEulerAngles());
				m_Scene->ConvertToLocaSpace(entity, bodyTransform);

				auto& transform = entity.Transform();
				transform.Translation = bodyTransform.Translation;
				transform.Rotation = bodyTransform.Rotation;
			}
		}
	}

	JPH::Body* PhysicsScene::CreateBody(Entity entity)
	{
		if (!entity.HasAny<SphereColliderComponent, BoxColliderComponent>())
		{
			SK_CORE_ERROR_TAG("Physics", "Creating a Body without a Collider is not allowed!");
			return nullptr;
		}

		JPH::BodyInterface& bodyInterface = m_System.GetBodyInterface();

		auto transform = m_Scene->GetWorldSpaceTransform(entity);
		auto& rigidbody = entity.GetComponent<RigidBodyComponent>();

		JPH::Ref<JPH::Shape> shape = CreateShape(entity, transform);

		JPH::BodyCreationSettings settings = JPH::BodyCreationSettings(shape,
																	   JoltUtils::ToJPH(transform.Translation),
																	   JoltUtils::ToJPH(transform.GetRotationQuat()),
																	   JoltUtils::GetMotionType(rigidbody.Type),
																	   rigidbody.Layer);
		
		settings.mUserData = entity.GetUUID();

		JPH::Body* body = bodyInterface.CreateBody(settings);
		JPH::BodyID bodyID = body->GetID();

		bodyInterface.AddBody(bodyID, JPH::EActivation::Activate);
		rigidbody.BodyID = bodyID;

		return body;
	}

	JPH::Ref<JPH::Shape> PhysicsScene::CreateShape(Entity entity, const TransformComponent& worldTransform)
	{
		if (entity.HasComponent<SphereColliderComponent>())
		{
			auto& collider = entity.GetComponent<SphereColliderComponent>();
			auto settings = JPH::SphereShapeSettings(collider.Radius * worldTransform.Scale.x, JPH::PhysicsMaterial::sDefault);
			//settings.SetDensity(collider.Density);

			auto result = settings.Create();
			if (result.HasError())
			{
				const JPH::String& errorMessage = result.GetError();
				SK_CORE_ERROR_TAG("Physics", "Failed to create Shape: {}", errorMessage);
				return nullptr;
			}

			return result.Get();
		}

		if (entity.HasComponent<BoxColliderComponent>())
		{
			auto& collider = entity.GetComponent<BoxColliderComponent>();
			auto settings = JPH::BoxShapeSettings(JoltUtils::ToJPH(collider.HalfSize * worldTransform.Scale));
			
			auto result = settings.Create();
			if (result.HasError())
			{
				const JPH::String& errorMessage = result.GetError();
				SK_CORE_ERROR_TAG("Physics", "Failed to create Shape: {}", errorMessage);
				return nullptr;
			}

			return result.Get();
		}

		SK_CORE_VERIFY(false, "Unkown Collider");
		return nullptr;
	}

}
