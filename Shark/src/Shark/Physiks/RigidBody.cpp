#include "skpch.h"
#include "RigidBody.h"

#include "Shark/Physiks/Collider.h"

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	RigidBody::RigidBody(b2Body* body)
		: m_Body(body)
	{
	}

	RigidBody::RigidBody(RigidBody&& other)
	{
		m_Body = other.m_Body;
		m_ColliderCount = other.m_ColliderCount;
		other.m_Body = nullptr;
		other.m_ColliderCount = 0;
	}

	RigidBody& RigidBody::operator=(RigidBody&& other)
	{
		m_Body = other.m_Body;
		m_ColliderCount = other.m_ColliderCount;
		other.m_Body = nullptr;
		other.m_ColliderCount = 0;

		return *this;
	}

	RigidBody::~RigidBody()
	{
		if (m_UserData)
			delete m_UserData;
	}

	BoxCollider RigidBody::CreateBoxCollider()
	{
		return CreateBoxCollider(ColliderSpecs{});
	}

	BoxCollider RigidBody::CreateBoxCollider(const ColliderSpecs& specs)
	{
		SK_PROFILE_FUNCTION();

		b2PolygonShape shape;
		shape.SetAsBox(specs.Width * 0.5f, specs.Height * 0.5f, { specs.Center.x, specs.Center.y }, specs.Rotation);
		b2FixtureDef def;
		def.shape = &shape;
		def.friction = specs.Friction;
		def.density = specs.Density;
		def.restitution = specs.Restitution;
		def.isSensor = specs.IsSensor;

		return BoxCollider(m_Body->CreateFixture(&def), m_ColliderCount++, specs);
	}

	void RigidBody::DestroyBoxCollider(BoxCollider& collider)
	{
		SK_PROFILE_FUNCTION();

		m_Body->DestroyFixture(collider);
		collider.m_Fixture = nullptr;
	}

	RigidBodySpecs RigidBody::GetCurrentState() const
	{
		SK_PROFILE_FUNCTION();

		RigidBodySpecs specs;
		specs.AllowSleep = IsSleepingAllowed();
		specs.Angle = GetAngle();
		specs.Awake = IsAwake();
		specs.Enabled = IsEnabled();
		specs.FixedRotation = IsFixedRoation();
		specs.Position = GetPosition();
		specs.Type = GetType();

		return specs;
	}

	void RigidBody::SetState(const RigidBodySpecs& specs)
	{
		SK_PROFILE_FUNCTION();

		SetSleepingAllowed(specs.AllowSleep);
		SetAngle(specs.Angle);
		SetAwake(specs.Awake);
		SetEnabled(specs.Enabled);
		SetFixedRotation(specs.FixedRotation);
		SetPosition(specs.Position.x, specs.Position.y);
		SetType(specs.Type);
	}

}