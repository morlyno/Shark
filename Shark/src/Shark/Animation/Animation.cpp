#include "skpch.h"
#include "Animation.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Animation/Skeleton.h"
#include "Shark/Render/MeshSource.h"

namespace Shark {

	Animation::Animation(const Skeleton* skeleton, std::vector<Channel> channels, float duration)
		: m_Channels(std::move(channels)), m_Skeleton(skeleton), m_Duration(duration)
	{
	}

	Scope<Pose> Animation::AllocatePose() const
	{
		auto pose = Pose::Allocate(m_Skeleton->GetBoneCount());
		InitializePose(*pose);
		return pose;
	}

	void Animation::InitializePose(Pose& pose, bool setIdentity) const
	{
		SK_CORE_VERIFY(pose.BoneCount == m_Skeleton->GetBoneCount());
		pose.Duration = m_Duration;
		pose.TimePosition = 0.0f;
		//pose.BoneCount = m_Skeleton->GetBoneCount();
		if (setIdentity)
		{
			std::ranges::fill(pose.GetBoneTransforms(), Transform::Identity());
		}
	}

	const Animation* AnimationAsset::GetAnimationAsync(bool wait) const
	{
		if (wait && !AssetManager::IsAssetLoaded(m_AnimationSource))
			AssetManager::WaitForAsset(m_AnimationSource, true);

		auto meshSource = AssetManager::GetAssetAsync<MeshSource>(m_AnimationSource);
		if (!meshSource)
			return nullptr;

		auto index = meshSource->FindAnimation(m_Name);
		if (!index)
			return nullptr;

		return &meshSource->GetAnimation(*index);
	}

}
