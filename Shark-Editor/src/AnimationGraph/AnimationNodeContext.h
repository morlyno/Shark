#pragma once

#include "NodeGraph/NodeContext.h"

namespace Shark {
	class Skeleton;
}

namespace Shark::NodeGraph {

	struct AnimationContextSpecification : public NodeContextSpecification
	{
		uint32_t BoneCount = 0;
		const Skeleton* Skeleton = nullptr;
	};

	class AnimationNodeContext : public NodeContext
	{
	public:
		AnimationNodeContext(const AnimationContextSpecification& specification);

		uint32_t        GetBoneCount() const { return m_BoneCount; }
		const Skeleton* GetSkeleton() const { return m_Skeleton; }

	private:
		uint32_t m_BoneCount = 0;
		const Skeleton* m_Skeleton;
	};

}
