#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Animation/Transform.h"

namespace Shark {
	class Skeleton;
	class MeshSource;
}

namespace Shark {

	template<typename T>
	struct KeyFrame
	{
		float FrameTime;
		T Value;
	};

	struct Channel
	{
		std::vector<KeyFrame<glm::vec3>> Translations;
		std::vector<KeyFrame<glm::quat>> Rotations;
		std::vector<KeyFrame<float>> Scales;
		size_t Index;
	};

	struct Pose
	{
		float Duration = 0.0f;
		float TimePosition = 0.0f;
		size_t BoneCount = 0;

		std::vector<Transform> BoneTransforms;
	};

	class Animation
	{
	public:
		Animation(const Skeleton* skeleton, std::vector<Channel> channels, float duration);

		float GetDuration() const { return m_Duration; }
		size_t GetFrameCount() const { return m_Channels.front().Translations.size(); }
		const auto* GetSkeleton() const { return m_Skeleton; }
		const auto& GetChannels() const { return m_Channels; }
		Pose* AllocatePose() const;
		void InitializePose(Pose& pose) const;

	private:
		const Skeleton* m_Skeleton = nullptr;
		std::vector<Channel> m_Channels;
		float m_Duration;

		friend class AssimpMeshImporter;
	};

	class AnimationAsset : public Asset
	{
	public:
		AnimationAsset() = default;
		~AnimationAsset() = default;

		const std::string& GetName() const { return m_Name; }
		AssetHandle GetAnimationSource() const { return m_AnimationSource; }
		AssetHandle GetSkeletonSource() const { return m_SkeletonSource; }

		// Can return nullptr when the MeshSource is not loaded yet
		const Animation* GetAnimationAsync(bool wait = false) const;
		const Skeleton* GetSkeletonAsync(bool wait = false) const;

	public:
		static AssetType GetStaticType() { return AssetType::Animation; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	private:
		AssetHandle m_AnimationSource;
		AssetHandle m_SkeletonSource;
		std::string m_Name;

		friend class AnimationSerializer;
		friend class AnimationEditor;
	};

}
