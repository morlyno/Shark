#include "skpch.h"
#include "PhysicsScene.h"

#include "Shark/Physics/3D/PhysicsSystem.h"
#include "Shark/Physics/3D/Utilities.h"
#include "Shark/Debug/Profiler.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <glm/gtx/optimum_pow.hpp>


namespace Shark {

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///// Broad Phase Layer Interface ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	const char* BroadPhaseLayerInterface::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
	{
		if (inLayer == 0)
			return "Default";

		JPH_ASSERT(false);
		return "Invalid";
	}
#endif

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///// Physics System ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	namespace utils {

		static JPH::Ref<JPH::Shape> CreateShape(Entity entity, const TransformComponent& worldTransform)
		{
			if (entity.HasComponent<BoxColliderComponent>())
			{
				auto& rigidbody = entity.GetComponent<RigidBodyComponent>();
				auto& collider = entity.GetComponent<BoxColliderComponent>();

				glm::vec3 halfSize = collider.HalfSize * worldTransform.Scale;
				glm::vec3 fullSize = halfSize * 2.0f;
				float volume = fullSize.x * fullSize.y * fullSize.z;

				auto settings = new JPH::BoxShapeSettings(JoltUtils::ToJPH(collider.HalfSize * worldTransform.Scale));
				settings->mDensity = rigidbody.Mass / volume;

				JPH::RotatedTranslatedShapeSettings offsetSettings(JoltUtils::ToJPH(collider.Offset), JPH::Quat::sIdentity(), settings);

				auto result = offsetSettings.Create();
				if (result.HasError())
				{
					const JPH::String& errorMessage = result.GetError();
					SK_CORE_ERROR_TAG("Physics", "Failed to create Shape: {}", errorMessage);
					return nullptr;
				}

				return result.Get();
			}

			if (entity.HasComponent<SphereColliderComponent>())
			{
				auto& rigidbody = entity.GetComponent<RigidBodyComponent>();
				auto& collider = entity.GetComponent<SphereColliderComponent>();

				float largest = glm::max(worldTransform.Scale.x, glm::max(worldTransform.Scale.y, worldTransform.Scale.z));
				float scaledRadius = collider.Radius * largest;
				float volume = (4.0f / 3.0f) * Math::PI * Math::Pow3(collider.Radius * worldTransform.Scale.x);

				auto settings = new JPH::SphereShapeSettings(scaledRadius, JPH::PhysicsMaterial::sDefault);
				settings->mDensity = rigidbody.Mass / volume;

				JPH::RotatedTranslatedShapeSettings offsetSettings(JoltUtils::ToJPH(collider.Offset), JPH::Quat::sIdentity(), settings);

				auto result = offsetSettings.Create();
				if (result.HasError())
				{
					const JPH::String& errorMessage = result.GetError();
					SK_CORE_ERROR_TAG("Physics", "Failed to create Shape: {}", errorMessage);
					return nullptr;
				}

				return result.Get();
			}

			if (entity.HasComponent<CapsuleColliderComponent>())
			{
				auto& rigidbody = entity.GetComponent<RigidBodyComponent>();
				auto& collider = entity.GetComponent<CapsuleColliderComponent>();

				float largest = glm::max(worldTransform.Scale.x, glm::max(worldTransform.Scale.y, worldTransform.Scale.z));
				float scaledRadius = collider.Radius * largest;
				float scaledHalfHeight = collider.HalfHeight * largest;

				float volume = Math::PI * glm::pow2(scaledRadius) * ((4.0f / 3.0f) * scaledRadius + scaledHalfHeight * 2.0f);

				auto settings = new JPH::CapsuleShapeSettings(scaledHalfHeight, scaledRadius);
				settings->mDensity = rigidbody.Mass / volume;

				auto offsetSettings = JPH::RotatedTranslatedShapeSettings(JoltUtils::ToJPH(collider.Offset), JPH::Quat::sIdentity(), settings);

				auto result = offsetSettings.Create();
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

		static JPH::BodyID CreateRigidBody(JPH::BodyInterface& bodyInterface, Ref<Scene> scene, Entity entity)
		{
			auto transform = scene->GetWorldSpaceTransform(entity);
			auto& rigidbody = entity.GetComponent<RigidBodyComponent>();

			auto shape = CreateShape(entity, transform);
			auto settings = JPH::BodyCreationSettings(shape,
													  JoltUtils::ToJPH(transform.Translation),
													  JoltUtils::ToJPH(transform.GetRotationQuat()),
													  JoltUtils::GetMotionType(rigidbody.Type),
													  JPH::ObjectLayer{ 0 });

			settings.mLinearDamping = rigidbody.LinearDrag;
			settings.mAngularDamping = rigidbody.AngularDrag;
			settings.mGravityFactor = rigidbody.DisableGravity ? 0.0f : 1.0f;
			settings.mIsSensor = rigidbody.IsSensor;

			settings.mLinearVelocity = JoltUtils::ToJPH(rigidbody.InitialLinearVelocity);
			settings.mAngularVelocity = JoltUtils::ToJPH(rigidbody.InitialAngularVelocity);

			settings.mMaxLinearVelocity = rigidbody.MaxLinearVelocity;
			settings.mMaxAngularVelocity = rigidbody.MaxAngularVelocity;

			settings.mMotionQuality = JoltUtils::GetMotionQuality(rigidbody.CollisionDetection);
			settings.mAllowedDOFs = JoltUtils::ConvertLockedAxesToAllowedDOFs(rigidbody.LockedAxes);

			settings.mUserData = entity.GetUUID();

			JPH::Body* joltBody = bodyInterface.CreateBody(settings);
			if (!joltBody)
			{
				SK_CORE_ERROR_TAG("Physics", "Failed to create JoltBody!");
				return {};
			}

			return joltBody->GetID();
		}

	}

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
			auto bodiesLock = JPH::BodyLockMultiRead(bodyLockInterface, activeBodies.data(), (int)activeBodies.size());
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

	void PhysicsScene::CreateBody(Entity entity)
	{
		if (!entity.HasAny<BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent>())
		{
			SK_CORE_ERROR_TAG("Physics", "Creating a Body without a Collider is not allowed!");
			return;
		}

		JPH::BodyInterface& bodyInterface = m_System.GetBodyInterface();
		JPH::BodyID bodyID = utils::CreateRigidBody(bodyInterface, m_Scene, entity);

		bodyInterface.AddBody(bodyID, JPH::EActivation::Activate);
		m_RigidBodyIDs[entity.GetUUID()] = bodyID;
	}

	void PhysicsScene::DestroyBody(Entity entity)
	{
		if (!m_RigidBodyIDs.contains(entity.GetUUID()))
		{
			return;
		}

		auto& bodyInterface = m_System.GetBodyInterface();
		JPH::BodyID bodyID = m_RigidBodyIDs.at(entity.GetUUID());

		bodyInterface.RemoveBody(bodyID);
		bodyInterface.DestroyBody(bodyID);
		m_RigidBodyIDs.erase(entity.GetUUID());
	}

}
