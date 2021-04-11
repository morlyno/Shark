#include "skpch.h"
#include "Collider.h"

namespace Shark {

	BoxCollider::BoxCollider(b2Fixture* fixture, uint32_t index, const ColliderSpecs& specs)
	{
		m_Fixture = fixture;
		m_Shape = specs.Shape;
		m_Width = specs.Width;
		m_Height = specs.Height;
		m_ColliderIndex = index;
		m_Center = specs.Center;
		m_Rotation = specs.Rotation;
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
		return specs;
	}

	void BoxCollider::SetState(const ColliderSpecs& specs)
	{
		SetDensity(specs.Density);
		SetFriction(specs.Friction);
		SetRestitution(specs.Restitution);
		//SetShape(specs.Shape);
		Resize(specs.Width, specs.Height);
	}

}