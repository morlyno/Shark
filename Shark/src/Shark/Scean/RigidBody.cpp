#include "skpch.h"
#include "RigidBody.h"

namespace Shark {

	RigidBody::RigidBody(b2Body* body, float width, float height)
		: m_Body(body), m_Width(width), m_Height(height)
	{
	}

	RigidBody::RigidBody(const RigidBody& other)
		: m_Body(other.m_Body), m_Width(other.m_Width), m_Height(other.m_Height)
	{
	}

	void RigidBody::Resize(float width, float height)
	{
		m_Width = width;
		m_Height = height;

		b2Fixture* f = m_Body->GetFixtureList();
		b2FixtureDef fd;
		if (f)
		{
			SK_CORE_ASSERT(f->GetType() == b2Shape::Type::e_polygon);
			fd.friction = f->GetFriction();
			fd.density = f->GetDensity();
			fd.restitution = f->GetRestitution();
		}
		m_Body->DestroyFixture(f);

		b2PolygonShape shape;
		shape.SetAsBox(width * 0.5f, height * 0.5f);

		fd.shape = &shape;

		m_Body->CreateFixture(&fd);
	}

	RigidBodySpecs RigidBody::GetCurrentState() const
	{
		RigidBodySpecs state;
		state.Position = GetPosition();
		state.Angle = GetAngle();
		state.Size = GetSize();

		state.Enabled = IsEnabled();
		state.Awake = IsAwake();
		state.AllowSleep = IsSleepingAllowed();
		state.FixedRotation = IsFixedRoation();

		state.Density = GetDensity();
		state.Friction = GetFriction();
		state.Restitution = GetRestituion();
		
		state.Type = GetType();
		state.Shape = GetShape();

		return state;
	}

	void RigidBody::SetState(const RigidBodySpecs& s)
	{
		SetPosition(s.Position.x, s.Position.y);
		SetAngle(s.Angle);
		Resize(s.Size.x, s.Size.y);

		SetEnabled(s.Enabled);
		SetAwake(s.Awake);
		SetSleepingAllowed(s.AllowSleep);
		SetFixedRotation(s.FixedRotation);

		SetDensity(s.Density);
		SetFriction(s.Friction);
		SetRestitution(s.Restitution);

		SetType(s.Type);
		//SetShape(s.Shape);
	}

}