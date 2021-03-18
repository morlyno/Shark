#pragma once

#include "Shark/Scean/RigidBody.h"

namespace Shark {

	struct RigidBodyComponent
	{
		RigidBodyComponent() = default;
		RigidBodyComponent(const RigidBody& body)
			: Body(body) {}
		~RigidBodyComponent() = default;

		RigidBody Body;
	};

}
