#pragma once

namespace Shark {

	enum class Collider2DType
	{
		BoxCollider,
		CircleCollider
	};

	enum class RigidbodyType
	{
		Static,
		Dynamic,
		Kinematic
	};

	enum class ContactType
	{
		CollisionBegin,
		CollisionEnd,
		TriggerBegin,
		TriggerEnd
	};

}
