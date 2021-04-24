#include "skpch.h"
#include "World.h"

#include "Shark/Physiks/RigidBody.h"
#include "Shark/Physiks/Collider.h"

namespace Shark {

	void CollisionDetector::BeginContact(b2Contact* contact)
	{
		auto& colliderA = *reinterpret_cast<BoxCollider*>(contact->GetFixtureA()->GetUserData().pointer);
		auto& colliderB = *reinterpret_cast<BoxCollider*>(contact->GetFixtureB()->GetUserData().pointer);
		colliderA.CollisionBegin(colliderB);
		colliderB.CollisionBegin(colliderA);
	}

	void CollisionDetector::EndContact(b2Contact* contact)
	{
		auto& colliderA = *reinterpret_cast<BoxCollider*>(contact->GetFixtureA()->GetUserData().pointer);
		auto& colliderB = *reinterpret_cast<BoxCollider*>(contact->GetFixtureB()->GetUserData().pointer);
		colliderA.CollisionEnd(colliderB);
		colliderB.CollisionEnd(colliderA);
	}
	
	World::World(const DirectX::XMFLOAT2& gravity)
	{
		m_World = new b2World({ gravity.x, gravity.y });
		m_World->SetContactListener(&m_CollisionDetector);
	}

	World::~World()
	{
		delete m_World;
	}

	World::World(World&& other)
	{
		SK_CORE_ASSERT(m_World == nullptr);
		m_World = other.m_World;
		m_CollisionDetector = other.m_CollisionDetector;
		m_World->SetContactListener(&m_CollisionDetector);

		other.m_World = nullptr;
	}

	World& World::operator=(World&& other)
	{
		if (m_World)
			delete m_World;
		m_World = other.m_World;
		m_CollisionDetector = other.m_CollisionDetector;
		m_World->SetContactListener(&m_CollisionDetector);

		other.m_World = nullptr;

		return *this;
	}

	void World::Update(float timeStep)
	{
		m_World->Step(timeStep, 8, 3);
	}

	void World::Flush()
	{
		b2Body* bodylist = m_World->GetBodyList();
		while (bodylist)
		{
			bodylist->SetLinearVelocity({ 0.0f, 0.0f });
			bodylist->SetAngularVelocity(0.0f);

			bodylist = bodylist->GetNext();
		}
	}

	RigidBody World::CreateRigidBody()
	{
		return CreateRigidBody({});
	}

	RigidBody World::CreateRigidBody(const RigidBodySpecs& specs)
	{
		b2BodyDef bodydef;
		bodydef.type = (b2BodyType)specs.Type;
		bodydef.allowSleep = specs.AllowSleep;
		bodydef.awake = specs.Awake;
		bodydef.enabled = specs.Enabled;
		bodydef.fixedRotation = specs.FixedRotation;
		bodydef.position = { specs.Position.x, specs.Position.y };
		bodydef.angle = specs.Angle;

		return RigidBody(m_World->CreateBody(&bodydef));
	}

	void World::DestroyRigidBody(RigidBody& rigidbody)
	{
		m_World->DestroyBody(rigidbody);
		rigidbody.m_Body = nullptr;
	}

}