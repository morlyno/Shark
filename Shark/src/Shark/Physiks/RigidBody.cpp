#include "skpch.h"
#include "RigidBody.h"

namespace Shark {

	RigidBody::RigidBody(b2Body* body) : m_Body(body) {}

#if 0
	RigidBody::RigidBody(const RigidBody& other)
	{
		b2World* w = other.m_Body->GetWorld();
		b2Body* b = other.m_Body;
		b2BodyDef def;
		def.type = b->GetType();
		def.position = b->GetPosition();
		def.angle = b->GetAngle();
		def.linearVelocity = b->GetLinearVelocity();
		def.angularVelocity = b->GetAngularVelocity();
		def.linearDamping = b->GetLinearDamping();
		def.angularDamping = b->GetAngularDamping();
		def.allowSleep = b->IsSleepingAllowed();
		def.awake = b->IsAwake();
		def.fixedRotation = b->IsFixedRotation();
		def.bullet = b->IsBullet();
		def.enabled = b->IsEnabled();
		def.userData = b->GetUserData();
		def.gravityScale = b->GetGravityScale();
		m_Body = w->CreateBody(&def);
	}

	RigidBody& RigidBody::operator=(const RigidBody& other)
	{
		b2World* w = other.m_Body->GetWorld();
		b2Body* b = other.m_Body;
		b2BodyDef def;
		def.type = b->GetType();
		def.position = b->GetPosition();
		def.angle = b->GetAngle();
		def.linearVelocity = b->GetLinearVelocity();
		def.angularVelocity = b->GetAngularVelocity();
		def.linearDamping = b->GetLinearDamping();
		def.angularDamping = b->GetAngularDamping();
		def.allowSleep = b->IsSleepingAllowed();
		def.awake = b->IsAwake();
		def.fixedRotation = b->IsFixedRotation();
		def.bullet = b->IsBullet();
		def.enabled = b->IsEnabled();
		def.userData = b->GetUserData();
		def.gravityScale = b->GetGravityScale();
		m_Body = w->CreateBody(&def);
		return *this;
	}
#endif

	BoxCollider RigidBody::CreateBoxCollider(const ColliderSpecs& specs)
	{
		b2PolygonShape shape;
		shape.SetAsBox(specs.Width * 0.5f, specs.Height * 0.5f, { specs.Center.x, specs.Center.y }, specs.Rotation);
		b2FixtureDef def;
		def.shape = &shape;
		def.friction = specs.Friction;
		def.density = specs.Density;
		def.restitution = specs.Restitution;

		return BoxCollider(m_Body->CreateFixture(&def), m_ColliderCount++, specs);
	}

	void RigidBody::DestroyBoxCollider(BoxCollider& collider)
	{
		m_Body->DestroyFixture(collider);
	}

	RigidBodySpecs RigidBody::GetCurrentState() const
	{
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
		SetSleepingAllowed(specs.AllowSleep);
		SetAngle(specs.Angle);
		SetAwake(specs.Awake);
		SetEnabled(specs.Enabled);
		SetFixedRotation(specs.FixedRotation);
		SetPosition(specs.Position.x, specs.Position.y);
		SetType(specs.Type);
	}

}