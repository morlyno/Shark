#pragma once

#include <box2d/box2d.h>
#include <DirectXMath.h>

namespace Shark {

	class CollisionDetector : public b2ContactListener
	{
	public:
		virtual ~CollisionDetector() = default;

		virtual void BeginContact(b2Contact* contact) override;

		virtual void EndContact(b2Contact* contact) override;

		//virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;


		// virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) override;
	};

	class RigidBody;
	struct RigidBodySpecs;

	class World
	{
	public:
		World(const DirectX::XMFLOAT2& gravity = { 0.0f, -10.0f });
		~World();

		World(const World& other) = delete;
		World& operator=(const World& other) = delete;

		World(World&& other);
		World& operator=(World&& other);

		void Update(float timeStep = 1.0f / 60.0f);
		void Flush();

		DirectX::XMFLOAT2 GetGravity() const { auto g = m_World->GetGravity(); return { g.x, g.y }; }
		void SetGravity(const DirectX::XMFLOAT2& gravity) { auto [x, y] = gravity; m_World->SetGravity({ x, y }); }

		RigidBody CreateRigidBody();
		RigidBody CreateRigidBody(const RigidBodySpecs& specs);
		void DestroyRigidBody(RigidBody& rigidbody);
	private:
		b2World* m_World = nullptr;
		CollisionDetector m_CollisionDetector;
	};

}
