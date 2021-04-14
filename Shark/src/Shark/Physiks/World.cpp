#include "skpch.h"
#include "World.h"

namespace Shark {
	
	World::World(const DirectX::XMFLOAT2& gravity)
	{
		m_World = new b2World({ gravity.x, gravity.y });
	}

	World::~World()
	{
		delete m_World;
	}

	World::World(World&& other)
	{
		m_World = other.m_World;
		other.m_World = nullptr;
	}

	World& World::operator=(World&& other)
	{
		m_World = other.m_World;
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
	}

}