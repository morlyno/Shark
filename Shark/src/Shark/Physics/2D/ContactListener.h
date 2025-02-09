#pragma once

#include "box2d/b2_contact.h"
#include <box2d/b2_world_callbacks.h>

namespace Shark {

	enum class ContactType
	{
		CollisionBegin,
		CollisionEnd,
		TriggerBegin,
		TriggerEnd
	};

	using ContactEventCallbackFn = std::function<void(ContactType, UUID entityAID, UUID entityBID)>;

	class ContactListener : public b2ContactListener
	{
	public:
		ContactListener(ContactEventCallbackFn contactEventCallback);

		virtual void BeginContact(b2Contact* contact) override;
		virtual void EndContact(b2Contact* contact) override;
		virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;
		virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;

	private:
		void OnCollisionBegin(b2Body* bodyA, b2Body* bodyB);
		void OnCollisionEnd(b2Body* bodyA, b2Body* bodyB);
		void OnTriggerBegin(b2Body* bodyA, b2Body* bodyB);
		void OnTriggerEnd(b2Body* bodyA, b2Body* bodyB);

	private:
		ContactEventCallbackFn m_ContactEventCallback;

	};

}
