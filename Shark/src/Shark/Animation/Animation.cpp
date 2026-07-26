#include "skpch.h"
#include "Animation.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/MeshSource.h"

namespace Shark {

	Animation::Animation(const Skeleton* skeleton, std::vector<Channel> channels, float duration)
		: m_Channels(std::move(channels)), m_Skeleton(skeleton), m_Duration(duration)
	{
	}

	const Animation* AnimationAsset::GetAnimationAsync() const
	{
		auto meshSource = AssetManager::GetAssetAsync<MeshSource>(m_AnimationSource);
		if (!meshSource)
			return nullptr;

		auto index = meshSource->FindAnimation(m_Name);
		if (!index)
			return nullptr;

		return &meshSource->GetAnimation(*index);
	}

	const Skeleton* AnimationAsset::GetSkeletonAsync() const
	{
		auto meshSource = AssetManager::GetAssetAsync<MeshSource>(m_SkeletonSource);
		if (!meshSource)
			return nullptr;

		return &meshSource->GetSkeleton();
	}

}
