#include "skpch.h"
#include "AnimationNodeContext.h"

#include "Shark/Scene/Scene.h"

namespace Shark::NodeGraph {

	AnimationNodeContext::AnimationNodeContext(const AnimationContextSpecification& specification)
		: NodeContext(specification), m_BoneCount(specification.BoneCount), m_Skeleton(specification.Skeleton)
	{

	}

}
