#pragma once

#include "Shark/NodeGraph/NodeContext.h"

namespace Shark {
	class Skeleton;
}

namespace Shark::NodeGraph {

	struct AnimationNodeContext : public NodeContext
	{
		uint32_t BoneCount = 0;
		const Skeleton* Skeleton;
	};

}
