#include "skpch.h"
#include "Collider.h"

#include "Shark/Physiks/RigidBody.h"

namespace Shark {

	BoxCollider::BoxCollider(b2Fixture* fixture, uint32_t index, const ColliderSpecs& specs)
	{
		SK_CORE_ASSERT(specs.Shape == GetShape());
		m_Fixture = fixture;
		m_Width = specs.Width;
		m_Height = specs.Height;
		m_ColliderIndex = index;
		m_Center = specs.Center;
		m_Rotation = specs.Rotation;
		m_Fixture->GetUserData().pointer = (uintptr_t)this;
	}

	BoxCollider::BoxCollider(BoxCollider&& other)
	{
		SK_CORE_ASSERT(other.GetShape() == GetShape());
		m_Fixture = other.m_Fixture;
		m_Width = other.m_Width;
		m_Height = other.m_Height;
		m_Center = other.m_Center;
		m_Rotation = other.m_Rotation;
		m_IsColliding = other.m_IsColliding;
		m_ColliderIndex = other.m_ColliderIndex;

		if (m_Fixture)
			m_Fixture->GetUserData().pointer = (uintptr_t)this;

		memset(&other, 0, sizeof(BoxCollider));
	}

	BoxCollider& BoxCollider::operator=(BoxCollider&& other)
	{
		SK_CORE_ASSERT(other.GetShape() == GetShape());
		m_Fixture = other.m_Fixture;
		m_Width = other.m_Width;
		m_Height = other.m_Height;
		m_Center = other.m_Center;
		m_Rotation = other.m_Rotation;
		m_IsColliding = other.m_IsColliding;
		m_ColliderIndex = other.m_ColliderIndex;

		if (m_Fixture)
			m_Fixture->GetUserData().pointer = (uintptr_t)this;

		memset(&other, 0, sizeof(BoxCollider));

		return *this;
	}

	BoxCollider::~BoxCollider()
	{
	}

	void BoxCollider::Resize(float width, float height)
	{
		SK_CORE_ASSERT(m_Fixture, "Fixture not Created");
		SK_CORE_ASSERT(m_Fixture->GetBody(), "Fixture not attached");
		SK_CORE_ASSERT(m_Fixture->GetType() == b2Shape::Type::e_polygon, "Other Shaped not implemented");

		m_Width = width;
		m_Height = height;

		b2Body* body = m_Fixture->GetBody();

		b2FixtureDef fd;
		fd.friction = m_Fixture->GetFriction();
		fd.density = m_Fixture->GetDensity();
		fd.restitution = m_Fixture->GetRestitution();
		fd.isSensor = m_Fixture->IsSensor();
		fd.userData.pointer = (uintptr_t)this;
		
		body->DestroyFixture(m_Fixture);

		b2PolygonShape shape;
		shape.SetAsBox(width * 0.5f, height * 0.5f, { m_Center.x, m_Center.y }, m_Rotation);

		fd.shape = &shape;

		m_Fixture = body->CreateFixture(&fd);
	}

	void BoxCollider::SetTransform(const DirectX::XMFLOAT2& center, float rotation)
	{
		SK_CORE_ASSERT(m_Fixture, "Fixture not Created");
		SK_CORE_ASSERT(m_Fixture->GetBody(), "Fixture not attached");
		SK_CORE_ASSERT(m_Fixture->GetType() == b2Shape::Type::e_polygon, "Other Shaped not implemented");

		m_Center = center;

		b2Body* body = m_Fixture->GetBody();

		b2FixtureDef fd;
		fd.friction = m_Fixture->GetFriction();
		fd.density = m_Fixture->GetDensity();
		fd.restitution = m_Fixture->GetRestitution();
		fd.isSensor = m_Fixture->IsSensor();
		fd.userData.pointer = (uintptr_t)this;

		body->DestroyFixture(m_Fixture);

		b2PolygonShape shape;
		shape.SetAsBox(m_Width * 0.5f, m_Height * 0.5f, { center.x, center.y }, rotation);

		fd.shape = &shape;

		m_Fixture = body->CreateFixture(&fd);
	}

	ColliderSpecs BoxCollider::GetCurrentState() const
	{
		ColliderSpecs specs;
		specs.Density = GetDensity();
		specs.Friction = GetFriction();
		specs.Restitution = GetRestituion();
		specs.Shape = GetShape();
		specs.Width = GetSize().x;
		specs.Height = GetSize().y;
		specs.IsSensor = IsSensor();
		return specs;
	}

	void BoxCollider::SetState(const ColliderSpecs& specs)
	{
		SK_CORE_ASSERT(specs.Shape == GetShape());
		SetDensity(specs.Density);
		SetFriction(specs.Friction);
		SetRestitution(specs.Restitution);
		SetSensor(specs.IsSensor);
		Resize(specs.Width, specs.Height);
		m_Fixture->GetUserData().pointer = (uintptr_t)this;
	}

	void BoxCollider::CollisionBegin(BoxCollider& other)
	{
		m_IsColliding = true;
	}

	void BoxCollider::CollisionEnd(BoxCollider& other)
	{
		m_IsColliding = false;
	}

}