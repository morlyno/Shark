#include "skpch.h"
#include "ContactListener.h"

namespace Shark {

	ContactListener::ContactListener(ContactEventCallbackFn contactEventCallback)
		: m_ContactEventCallback(contactEventCallback)
	{
	}

	void ContactListener::BeginContact(b2Contact* contact)
	{
		b2Fixture* fixtureA = contact->GetFixtureA();
		b2Fixture* fixtureB = contact->GetFixtureB();
		if (fixtureA->IsSensor() || fixtureB->IsSensor())
			OnTriggerBegin(fixtureA->GetBody(), fixtureB->GetBody());
		else
			OnCollisionBegin(fixtureA->GetBody(), fixtureB->GetBody());
	}

	void ContactListener::EndContact(b2Contact* contact)
	{
		b2Fixture* fixtureA = contact->GetFixtureA();
		b2Fixture* fixtureB = contact->GetFixtureB();
		if (fixtureA->IsSensor() || fixtureB->IsSensor())
			OnTriggerEnd(fixtureA->GetBody(), fixtureB->GetBody());
		else
			OnCollisionEnd(fixtureA->GetBody(), fixtureB->GetBody());
	}

	void ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
	{

	}

	void ContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
	{

	}

	void ContactListener::OnCollisionBegin(b2Body* bodyA, b2Body* bodyB)
	{
		m_ContactEventCallback(ContactType::CollisionBegin, bodyA->GetUserData().pointer, bodyB->GetUserData().pointer);
	}

	void ContactListener::OnCollisionEnd(b2Body* bodyA, b2Body* bodyB)
	{
		m_ContactEventCallback(ContactType::CollisionEnd, bodyA->GetUserData().pointer, bodyB->GetUserData().pointer);
	}

	void ContactListener::OnTriggerBegin(b2Body* bodyA, b2Body* bodyB)
	{
		m_ContactEventCallback(ContactType::TriggerBegin, bodyA->GetUserData().pointer, bodyB->GetUserData().pointer);
	}

	void ContactListener::OnTriggerEnd(b2Body* bodyA, b2Body* bodyB)
	{
		m_ContactEventCallback(ContactType::TriggerEnd, bodyA->GetUserData().pointer, bodyB->GetUserData().pointer);
	}

}
