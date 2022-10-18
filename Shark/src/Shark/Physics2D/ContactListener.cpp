#include "skpch.h"
#include "ContactListener.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"

#include "Shark/Physics2D/PhysicsScene.h"
#include "Shark/Scripting/ScriptGlue.h"

#include <box2d/b2_contact.h>

namespace Shark {

	void ContactListener::BeginContact(b2Contact* contact)
	{
		b2Fixture* fixtureA = contact->GetFixtureA();
		b2Fixture* fixtureB = contact->GetFixtureB();

		Ref<Physics2DActor> actorA = (Physics2DActor*)fixtureA->GetUserData().pointer;
		Ref<Physics2DActor> actorB = (Physics2DActor*)fixtureB->GetUserData().pointer;

		Entity entityA = actorA->GetEntity();
		Entity entityB = actorB->GetEntity();

		b2Shape* shapeA = fixtureA->GetShape();
		b2Shape* shapeB = fixtureB->GetShape();

		Collider2DType colliderTypeA = shapeA->GetType() == b2Shape::e_polygon ? Collider2DType::BoxCollider : Collider2DType::CircleCollider;
		Collider2DType colliderTypeB = shapeB->GetType() == b2Shape::e_polygon ? Collider2DType::BoxCollider : Collider2DType::CircleCollider;

		ScriptGlue::CallCollishionBegin(entityA, entityB, colliderTypeB, fixtureB->IsSensor());
		ScriptGlue::CallCollishionBegin(entityB, entityA, colliderTypeA, fixtureA->IsSensor());
	}

	void ContactListener::EndContact(b2Contact* contact)
	{
		b2Fixture* fixtureA = contact->GetFixtureA();
		b2Fixture* fixtureB = contact->GetFixtureB();

		Ref<Physics2DActor> actorA = (Physics2DActor*)fixtureA->GetUserData().pointer;
		Ref<Physics2DActor> actorB = (Physics2DActor*)fixtureB->GetUserData().pointer;

		Entity entityA = actorA->GetEntity();
		Entity entityB = actorB->GetEntity();

		b2Shape* shapeA = fixtureA->GetShape();
		b2Shape* shapeB = fixtureB->GetShape();

		Collider2DType colliderTypeA = shapeA->GetType() == b2Shape::e_polygon ? Collider2DType::BoxCollider : Collider2DType::CircleCollider;
		Collider2DType colliderTypeB = shapeB->GetType() == b2Shape::e_polygon ? Collider2DType::BoxCollider : Collider2DType::CircleCollider;

		ScriptGlue::CallCollishionEnd(entityA, entityB, colliderTypeB, fixtureB->IsSensor());
		ScriptGlue::CallCollishionEnd(entityB, entityA, colliderTypeA, fixtureA->IsSensor());
	}

	void ContactListener::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
	}

}
