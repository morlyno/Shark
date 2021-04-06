#pragma once

#include "Shark/Physiks/Collider.h"

namespace Shark {

	struct BoxColliderComponent
	{
		BoxColliderComponent() = default;
		BoxColliderComponent(const BoxCollider& collider)
			: Collider(collider) {}
		~BoxColliderComponent() = default;

		BoxCollider Collider;
	};

}
