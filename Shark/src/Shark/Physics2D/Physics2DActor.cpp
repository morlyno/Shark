#include "skpch.h"
#include "Physics2DActor.h"

#include "Shark/Physics2D/Physics2D.h"
#include "Shark/Physics2D/Physics2DUtils.h"

#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>
#include "Shark/Math/Math.h"

namespace Shark {

	namespace utils {

		b2BodyType ToB2BodyType(RigidBody2DType bodyType)
		{
			switch (bodyType)
			{
				case RigidBody2DType::Static: return b2_staticBody;
				case RigidBody2DType::Dynamic: return b2_dynamicBody;
				case RigidBody2DType::Kinematic: return b2_kinematicBody;
			}
			SK_CORE_ASSERT(false, "Unkown RigidBody2DType");
			return b2_staticBody;
		}

	}

	Physics2DActor::~Physics2DActor()
	{
	}

	Entity Physics2DActor::GetEntity() const
	{
		return m_Entity;
	}

	void Physics2DActor::AddCollider(BoxCollider2DComponent& collider)
	{
		Ref<Scene> entityScene = m_Entity.GetScene();
		auto transform = entityScene->GetWorldSpaceTransform(m_Entity);

		b2PolygonShape shape;
		shape.SetAsBox(collider.Size.x * transform.Scale.x, collider.Size.y * transform.Scale.y, { collider.Offset.x, collider.Offset.y }, collider.Rotation);

		b2FixtureDef def;
		def.shape = &shape;
		def.friction = collider.Friction;
		def.density = collider.Density;
		def.restitution = collider.Restitution;
		def.restitutionThreshold = collider.RestitutionThreshold;
		def.isSensor = collider.IsSensor;
		def.userData.pointer = (uintptr_t)this;
		collider.RuntimeCollider = m_Body->CreateFixture(&def);
	}

	void Physics2DActor::AddCollider(CircleCollider2DComponent& collider)
	{
		Ref<Scene> entityScene = m_Entity.GetScene();
		auto transform = entityScene->GetWorldSpaceTransform(m_Entity);

		b2CircleShape shape;
		shape.m_radius = collider.Radius * transform.Scale.x;
		shape.m_p = { collider.Offset.x, collider.Offset.y };

		b2FixtureDef def;
		def.shape = &shape;
		def.friction = collider.Friction;
		def.density = collider.Density;
		def.restitution = collider.Restitution;
		def.restitutionThreshold = collider.RestitutionThreshold;
		def.isSensor = collider.IsSensor;
		def.userData.pointer = (uintptr_t)this;
		collider.RuntimeCollider = m_Body->CreateFixture(&def);
	}

	void Physics2DActor::RemoveCollider(BoxCollider2DComponent& collider)
	{
		m_Body->DestroyFixture(collider.RuntimeCollider);
		collider.RuntimeCollider = nullptr;
	}

	void Physics2DActor::RemoveCollider(CircleCollider2DComponent& collider)
	{
		m_Body->DestroyFixture(collider.RuntimeCollider);
		collider.RuntimeCollider = nullptr;
	}

	void Physics2DActor::SetBodyType(RigidBody2DType bodyType)
	{
		b2BodyType b2Type = utils::ToB2BodyType(bodyType);
		m_Body->SetType(b2Type);
	}

	bool Physics2DActor::IsStatic() const
	{
		return m_Body->GetType() == b2_staticBody;
	}

	void Physics2DActor::SyncronizeTransforms()
	{
		if (m_Entity.GetName() == "Platform")
		{
			int i = 0;
		}

		Ref<Scene> entityScene = m_Entity.GetScene();
		auto transformMatrix = Physics2DUtils::GetMatrix(this);
		entityScene->ConvertToLocaSpace(m_Entity, transformMatrix);
		TransformComponent transform;
		Math::DecomposeTransform(transformMatrix, transform);
		auto& entityTransform = m_Entity.Transform();
		entityTransform.Translation.x = transform.Translation.x;
		entityTransform.Translation.y = transform.Translation.y;
		entityTransform.Rotation.z = transform.Rotation.z;
	}

	RigidBody2DType Physics2DActor::GetBodyType() const
	{
		switch (m_Body->GetType())
		{
			case b2_staticBody: return RigidBody2DType::Static;
			case b2_kinematicBody: return RigidBody2DType::Kinematic;
			case b2_dynamicBody: return RigidBody2DType::Dynamic;
		}

		SK_CORE_ASSERT(false, "Unkown b2BodyType");
		return RigidBody2DType::None;
	}

}
