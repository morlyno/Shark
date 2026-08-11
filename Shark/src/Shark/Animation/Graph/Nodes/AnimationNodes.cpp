#include "skpch.h"
#include "AnimationNodes.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Animation/Animation.h"

#include "Shark/Animation/Graph/AnimationNodeContext.h"
#include "Shark/Animation/Graph/PinTypes.h"

namespace Shark::NodeGraph {

	Nodes::AnimationPlayer::AnimationPlayer(UUID id, NodeContext* context)
		: ProcessNode(id), Pose(CreateTypePose(static_cast<AnimationNodeContext*>(context)->GetBoneCount()))
	{
		Details::RegisterVariables(this);
	}

	void Nodes::AnimationPlayer::Initialize(NodeContext* context)
	{
		Details::InitializeInputs(this);
	}

	void Nodes::AnimationPlayer::Process(float ts)
	{
		if (m_ActiveAnimation != *in_Animation)
		{
			if (auto animation = AssetManager::GetAsset<AnimationAsset>(AssetHandle::Make(*in_Animation)))
				m_Animation = animation->GetAnimationAsync(true);

			m_TimePosition = 0.0f;
			m_ActiveAnimation = AssetHandle::Make(*in_Animation);
			m_Finished = false;
		}

		if (m_Animation && !m_Finished)
		{
			m_TimePosition += ts / m_Animation->GetDuration();
			if (*Loop)
			{
				int loopCount = static_cast<int>(glm::floor(m_TimePosition));
				m_TimePosition -= loopCount;

				for (int i = 0; i < loopCount; i++)
					OnLoop();
			}
			else if (m_TimePosition > 1.0f)
			{
				m_TimePosition = 1.0f;
				m_Finished = true;
				OnFinish();
			}

			const float frameInterval = 1.0f / static_cast<float>(m_Animation->GetFrameCount() - 1);
			const size_t sampleIndex = static_cast<size_t>(m_TimePosition * (m_Animation->GetFrameCount() - 1));

			const size_t startIndex = sampleIndex;
			const size_t endIndex = *Loop ? (sampleIndex + 1) % m_Animation->GetFrameCount() : std::min(sampleIndex + 1, m_Animation->GetFrameCount() - 1);

			auto* pose = static_cast<Types::IPose*>(Pose.getRawData());
			auto* boneTransforms = pose->GetBoneTransforms();

			const auto& channels = m_Animation->GetChannels();
			for (size_t i = 0; i < channels.size(); i++)
			{
				const auto& channel = channels[i];
				const float t = (m_TimePosition - channel.Translations[startIndex].FrameTime) / frameInterval;

				const auto lerp  = [t, startIndex, endIndex](const auto& frames) { return glm::mix(frames[startIndex].Value, frames[endIndex].Value, t); };
				const auto slerp = [t, startIndex, endIndex](const auto& frames) { return glm::slerp(frames[startIndex].Value, frames[endIndex].Value, t); };

				boneTransforms[i].Translation = lerp(channel.Translations);
				boneTransforms[i].Rotation    = slerp(channel.Rotations);
				boneTransforms[i].Scale       = lerp(channel.Scales);
			}
		}
	}

}
