#include "skpch.h"
#include "AnimationGraphContext.h"

#include "AnimationGraph/AnimationFactory.h"

namespace Shark::NodeGraph::Editor {

	AnimationGraphContext::AnimationGraphContext()
	{
		m_Factory = Scope<AnimationFactory>::Create();
	}

	AnimationGraphContext::~AnimationGraphContext()
	{

	}

	AssetType AnimationGraphContext::GetPinAssetType(const Pin* pin) const
	{
		const Node* node = FindNode(pin->NodeID);

		if (node->Name == "AnimationPlayer" && pin->Name == "Animation")
			return AssetType::Animation;

		return AssetType::None;
	}

}
