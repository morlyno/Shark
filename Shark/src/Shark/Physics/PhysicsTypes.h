#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	enum class BodyType
	{
		Static,
		Dynamic,
		Kinematic
	};

	enum class CollisionDetectionType
	{
		Discrete,
		Continuous
	};

	enum class Axis : uint32_t
	{
		None = 0,
		TranslationX = BIT(0), TranslationY = BIT(1), TranslationZ = BIT(2),
		RotationX = BIT(3), RotationY = BIT(4), RotationZ = BIT(5),

		Translation = TranslationX | TranslationY | TranslationZ,
		Rotation = RotationX | RotationY | RotationZ,
		All = Translation | Rotation,
	};

	using PhysicsLayer = uint16_t;

	namespace Physics3D {

		struct DefaultLayers
		{
			enum : PhysicsLayer
			{
				NonMoving = 0,
				Moving = 1
			};
		};

	}

}

template <>
struct ::magic_enum::customize::enum_range<Shark::Axis> {
	static constexpr bool is_flags = true;
};
