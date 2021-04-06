#pragma once

#include <box2d/box2d.h>
#include <DirectXMath.h>

#include "Shark/Physiks/RigidBody.h"

namespace Shark {

	class World
	{
	public:
		World(const DirectX::XMFLOAT2& gravity = { 0.0f, -10.0f });

		void Update(float timeStep = 1.0f / 60.0f);
		void Flush();

		RigidBody CreateRigidBody(const RigidBodySpecs& specs = RigidBodySpecs{});
		void DestroyRigidBody(RigidBody& rigidbody);
	private:
		b2World m_World;
	};

}
