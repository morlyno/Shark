#include "skpch.h"
#include "Animation.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/MeshSource.h"

namespace Shark {

	Animation::Animation(const Skeleton* skeleton, std::vector<Channel> channels, float duration)
		: m_Channels(std::move(channels)), m_Skeleton(skeleton), m_Duration(duration)
	{
	}

	Pose* Animation::AllocatePose() const
	{
		auto pose = sknew Pose();
		InitializePose(*pose);
		return pose;
	}

	void Animation::InitializePose(Pose& pose) const
	{
		pose.Duration = m_Duration;
		pose.TimePosition = 0.0f;
		pose.BoneCount = m_Skeleton->GetBoneCount();
		pose.BoneTransforms.resize(pose.BoneCount, Transform::Identity());
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

	const Skeleton* AnimationAsset::GetSkeletonAsync(bool wait) const
	{
		if (wait && !AssetManager::IsAssetLoaded(m_SkeletonSource))
			AssetManager::WaitForAsset(m_SkeletonSource, true);

		auto meshSource = AssetManager::GetAssetAsync<MeshSource>(m_SkeletonSource);
		if (!meshSource)
			return nullptr;

		return &meshSource->GetSkeleton();
	}

}
