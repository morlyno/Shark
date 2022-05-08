#pragma once

class b2Body;

namespace Shark {

	struct RigidBody2DComponent
	{
		enum class BodyType { Static = 0, Dynamic = 1, Kinematic = 2 };
		BodyType Type = BodyType::Dynamic;
		bool FixedRotation = false;
		bool IsBullet = false;
		bool Awake = true;
		bool Enabled = true;
		float GravityScale = 1.0f;

		b2Body* RuntimeBody = nullptr;

		RigidBody2DComponent() = default;
		RigidBody2DComponent(const RigidBody2DComponent&) = default;
	};

}
