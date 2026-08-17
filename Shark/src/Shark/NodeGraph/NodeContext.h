#pragma once

#include "Shark/Core/Base.h"

#include <random>

#if TODO
namespace Shark {
	class Scene;
}
#endif

namespace Shark::NodeGraph {

	struct NodeContext
	{
		int Domain = 0;

		std::mt19937 RandomEngine;
		uint32_t Seed = std::mt19937::default_seed;

#if TODO
		Ref<Scene> Scene;
#endif
	};

}
