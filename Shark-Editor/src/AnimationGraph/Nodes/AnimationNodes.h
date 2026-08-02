#pragma once

#include "Shark/Asset/AssetTypes.h"

#include "NodeGraph/ProcessNode.h"
#include "NodeGraph/PinTypes.h"
#include "AnimationGraph/PinTypes.h"

namespace Shark {
	class Animation;
	class Skeleton;
}

namespace Shark::NodeGraph {

	namespace Nodes {

		struct AnimationPlayer : public ProcessNode
		{
			Types::AssetHandle* in_Animation = nullptr;
			bool* Loop = nullptr;

			OutputEvent OnLoop;
			OutputEvent OnFinish;
			choc::value::Value Pose;

		public:
			AnimationPlayer(UUID id, NodeContext* context);
			virtual void Initialize(NodeContext* context) override;
			virtual void Process(float ts) override;

		private:
			AssetHandle m_ActiveAnimation;
			const Animation* m_Animation = nullptr;
			const Skeleton* m_Skeleton = nullptr;

			float m_TimePosition = 0.0f;
			bool m_Finished = false;
		};

	}

}

REFLECT_NODE(
	Shark::NodeGraph::Nodes::AnimationPlayer,
	REFLECT_INPUTS(&Shark::NodeGraph::Nodes::AnimationPlayer::in_Animation,
				   &Shark::NodeGraph::Nodes::AnimationPlayer::Loop),
	REFLECT_OUTPUTS(&Shark::NodeGraph::Nodes::AnimationPlayer::Pose,
					&Shark::NodeGraph::Nodes::AnimationPlayer::OnLoop,
					&Shark::NodeGraph::Nodes::AnimationPlayer::OnFinish)
);
