#pragma once

#include <box2d/box2d.h>
#include <DirectXMath.h>

#include "Shark/Physiks/RigidBody.h"

namespace Shark {

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

		RigidBody CreateRigidBody(const RigidBodySpecs& specs = RigidBodySpecs{});
		void DestroyRigidBody(RigidBody& rigidbody);
	private:
		b2World* m_World = nullptr;
	};

}
