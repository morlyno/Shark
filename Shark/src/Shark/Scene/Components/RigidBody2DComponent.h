#pragma once

class b2Body;

namespace Shark {

	struct RigidBody2DComponent
	{
		enum class BodyType { Static = 0, Dynamic = 1, Kinematic = 2 };
		BodyType Type = BodyType::Dynamic;
		bool FixedRotation = false;

		b2Body* RuntimeBody = nullptr;

		RigidBody2DComponent() = default;
		RigidBody2DComponent(const RigidBody2DComponent&) = default;
	};

}
