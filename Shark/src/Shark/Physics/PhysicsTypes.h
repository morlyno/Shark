#pragma once

namespace Shark {

	enum class BodyType
	{
		Static,
		Dynamic,
		Kinematic
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
