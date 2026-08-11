#include "skpch.h"
#include "AnimationEngine.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Animation/Animation.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Utils/Utilities.h"

namespace Shark {

	AnimationEngine::AnimationEngine(Ref<Scene> scene)
		: m_CurrentScene(scene.Raw())
	{
	}

	void AnimationEngine::SetCurrentScene(Ref<Scene> scene)
	{
		m_CurrentScene = scene.Raw();
	}

	void AnimationEngine::RegisterEntity(const Entity& entity)
	{
		auto& component = entity.GetComponent<AnimationComponent>();
		auto animation = AssetManager::GetAsset<AnimationAsset>(component.Animation);
		if (!animation)
			return;

		auto& entry = m_RegisteredAnimations.emplace_back();
		entry.EntityID = entity.GetUUID();
		entry.ActiveAnimation = component.Animation;
		entry.Animation = animation->GetAnimationAsync(true);
		entry.Skeleton = entry.Animation->GetSkeleton();
		SK_CORE_VERIFY(entry.Animation && entry.Skeleton);
		entry.Pose = entry.Animation->AllocatePose();
		entry.Update = component.Update;
		entry.Loop = component.Loop;
	}

	void AnimationEngine::Transition(UUID targetEntityID, AssetHandle animationHandle, float duration, bool loop)
	{
		auto animation = AssetManager::GetAsset<AnimationAsset>(animationHandle);
		if (!animation)
			return;

		size_t target = ~0;
		for (size_t i = 0; i < m_RegisteredAnimations.size(); i++)
		{
			auto& entry = m_RegisteredAnimations[i];
			if (entry.EntityID != targetEntityID && !entry.IsTransition)
				continue;

			target = i;
			break;
		}

		if (target == ~0)
			return;

		const size_t transitionIndex = m_RegisteredAnimations.size();

		auto& entry = m_RegisteredAnimations.emplace_back();
		entry.EntityID        = targetEntityID;
		entry.ActiveAnimation = animationHandle;
		entry.Animation       = animation->GetAnimationAsync(true);
		entry.Skeleton        = entry.Animation->GetSkeleton();
		SK_CORE_VERIFY(entry.Animation && entry.Skeleton);
		entry.Pose            = entry.Animation->AllocatePose();
		entry.Update          = true;
		entry.Loop            = loop;
		entry.IsTransition    = true;
		
		auto& transition = m_Transitions.emplace_back();
		transition.Blend      = 0.0f;
		transition.Duration   = duration;
		transition.Target     = static_cast<uint32_t>(target);
		transition.Transition = static_cast<uint32_t>(transitionIndex);
	}

	void AnimationEngine::Update(TimeStep ts)
	{
		UpdateEntries();

		for (auto& entry : m_RegisteredAnimations)
		{
			UpdateAnimation(ts, entry);
		}

		for (auto& transition : m_Transitions)
		{
			auto& target = m_RegisteredAnimations[transition.Target];
			auto& source = m_RegisteredAnimations[transition.Transition];

			transition.Blend += ts / transition.Duration;
			if (transition.Blend > 1)
			{
				target.ActiveAnimation = source.ActiveAnimation;
				target.Animation       = source.Animation;
				target.Skeleton        = source.Skeleton;
				target.Pose            = std::move(source.Pose);
				target.SamplePosition  = source.SamplePosition;
				target.Loop            = source.Loop;

				// update component
				Entity entity = m_CurrentScene->GetEntityByID(target.EntityID);
				auto& component = entity.GetComponent<AnimationComponent>();

				component.Animation = target.ActiveAnimation;
				component.Loop      = target.Loop;

				// mark source as obsolete
				source.EntityID = UUID::Invalid;
				continue;
			}

			for (size_t i = 0; i < target.Pose->BoneCount; i++)
			{
				auto& transform = target.Pose->BoneTransforms[i];
				auto& blendSource = source.Pose->BoneTransforms[i];

				transform.Translation = glm::mix(transform.Translation, blendSource.Translation, transition.Blend);
				transform.Rotation = glm::slerp(transform.Rotation, blendSource.Rotation, transition.Blend);
				transform.Scale = glm::mix(transform.Scale, blendSource.Scale, transition.Blend);
			}
		}

	}

	const Pose* AnimationEngine::GetPose(UUID entityID) const
	{
		const auto i = std::ranges::find(m_RegisteredAnimations, entityID, &AnimationEntry::EntityID);
		if (i == m_RegisteredAnimations.end())
			return nullptr;

		return i->Pose.Raw();
	}

	void AnimationEngine::SetSamplePosition(UUID entityID, float position)
	{
		const auto i = std::ranges::find(m_RegisteredAnimations, entityID, &AnimationEntry::EntityID);
		if (i == m_RegisteredAnimations.end())
			return;

		i->SamplePosition = position;
	}

	AnimationEngine::PoseIterator AnimationEngine::GetPoses()
	{
		return PoseIterator(*this);
	}

	void AnimationEngine::OnAssetReloaded(AssetHandle handle)
	{
		auto assetType = AssetManager::GetAssetType(handle);
		auto registeredEntries = AsReferenceRange(m_RegisteredAnimations/*, m_Transitions*/);

		for (auto& array : registeredEntries)
		{
			for (auto& entry : *array)
			{
				switch (assetType)
				{
					case AssetType::MeshSource:
					{
						auto animation = AssetManager::GetAsset<AnimationAsset>(entry.ActiveAnimation);
						if (handle == animation->GetAnimationSource())
						{
							entry.Animation = nullptr;
							entry.Skeleton = nullptr;
						}
						break;
					}

					case AssetType::Animation:
					{
						if (entry.ActiveAnimation == handle)
						{
							entry.Animation = nullptr;
							entry.Skeleton = nullptr;
						}
						break;
					}
				}
			}
		}
	}

	void AnimationEngine::UpdateEntries()
	{
		bool reindex = false;

		for (auto& entry : m_RegisteredAnimations)
		{
			auto entity = m_CurrentScene->TryGetEntityByUUID(entry.EntityID);
			if (!entity || !entity.HasComponent<AnimationComponent>())
			{
				entry.EntityID = UUID::Invalid;
				reindex = true;
				continue;
			}

			const auto& component = entity.GetComponent<AnimationComponent>();
			const auto currentAnimation = entry.IsTransition ? entry.ActiveAnimation : component.Animation;

			if (!currentAnimation)
			{
				entry.Animation = nullptr;
				entry.Skeleton  = nullptr;
				continue;
			}

			if (currentAnimation != entry.ActiveAnimation || !entry.Animation || !entry.Skeleton)
			{
				auto animation = AssetManager::GetAsset<AnimationAsset>(currentAnimation);
				entry.Animation = animation->GetAnimationAsync(true);
				entry.Skeleton = entry.Animation->GetSkeleton();
				SK_CORE_VERIFY(entry.Animation);
				entry.Animation->InitializePose(*entry.Pose);
				entry.ActiveAnimation = currentAnimation;
			}

			if (!entry.IsTransition)
			{
				entry.Update = component.Update;
				entry.Loop = component.Loop;
			}
		}

		if (reindex)
		{
			size_t count = 0;
			for (size_t i = 0; i < (m_RegisteredAnimations.size() - count); i++)
			{
				if (m_RegisteredAnimations[i].EntityID != UUID::Invalid)
					continue;

				const size_t swapTarget = m_RegisteredAnimations.size() - ++count;

				std::swap(m_RegisteredAnimations[i], m_RegisteredAnimations[swapTarget]);

				for (size_t j = 0; j < m_Transitions.size();)
				{
					auto& transition = m_Transitions[j];
					if (transition.Target == i || transition.Transition == i)
					{
						m_Transitions.erase(m_Transitions.begin() + j);
						continue;
					}

					if (transition.Target == swapTarget)
						transition.Target = i;

					if (transition.Transition == swapTarget)
						transition.Transition = i;

					j += 1;
				}
			}

			m_RegisteredAnimations.erase(m_RegisteredAnimations.begin() + (m_RegisteredAnimations.size() - count), m_RegisteredAnimations.end());
		}
	}

	void AnimationEngine::UpdateAnimation(TimeStep ts, AnimationEntry& entry)
	{
		if ((!entry.Update || (!entry.Loop && entry.Pose->TimePosition >= 1.0f)) && entry.SamplePosition == entry.Pose->TimePosition)
			return;

		float targetTimePosition = entry.SamplePosition + ts / entry.Animation->GetDuration();

		if (targetTimePosition > 1.0f)
		{
			if (entry.Loop)
			{
				targetTimePosition -= glm::floor(targetTimePosition);
			}
			else
			{
				targetTimePosition = 1.0f;
			}
		}

		entry.Pose->TimePosition = targetTimePosition;
		entry.SamplePosition     = targetTimePosition;

		const size_t index = static_cast<size_t>(targetTimePosition * (entry.Animation->GetFrameCount() - 1));
		SK_CORE_ASSERT(index < entry.Animation->GetFrameCount());

		const float frameInterval = 1.0f / static_cast<float>(entry.Animation->GetFrameCount() - 1);
		const size_t startIndex = index;
		const size_t endIndex = entry.Loop ? // wrap or clamp end index
			(index + 1) % entry.Animation->GetFrameCount() :
			std::min(index + 1, entry.Animation->GetFrameCount() - 1);

		const auto& channels = entry.Animation->GetChannels();
		for (size_t i = 0; i < channels.size(); i++)
		{
			const Channel& channel = channels[i];
			const float t = (targetTimePosition - channel.Translations[startIndex].FrameTime) / frameInterval;

			const auto mix = [t, startIndex, endIndex](const auto& frames) { return glm::mix(frames[startIndex].Value, frames[endIndex].Value, t); };
			// Spherical linear interpolation for quaternions because mix/lerp is oriented
			const auto mixQuat = [t, startIndex, endIndex](const auto& frames) { return glm::slerp(frames[startIndex].Value, frames[endIndex].Value, t); };

			auto& transform = entry.Pose->BoneTransforms[i];
			transform.Translation = mix(channel.Translations);
			transform.Rotation = mixQuat(channel.Rotations);
			transform.Scale = mix(channel.Scales);
		}

	}

	std::pair<UUID, Pose*> AnimationEngine::GetEntityAndPose(size_t index)
	{
		if (index >= m_RegisteredAnimations.size())
			return { UUID::Invalid, nullptr };

		return {
			m_RegisteredAnimations[index].EntityID,
			m_RegisteredAnimations[index].Pose.Raw()
		};
	}

	void AnimationEngine::AdvanceIterator(PoseIterator& iterator)
	{
		while (++iterator.m_Index < m_RegisteredAnimations.size())
		{
			auto& entry = m_RegisteredAnimations[iterator.m_Index];
			if (entry.IsTransition || !entry.EntityID)
				continue;

			iterator.m_Entry = GetEntityAndPose(iterator.m_Index);
			return;
		}
	}

	AnimationEngine::PoseIterator& AnimationEngine::PoseIterator::operator++()
	{
		m_Engine->AdvanceIterator(*this);
		return *this;
	}

	AnimationEngine::PoseIterator AnimationEngine::PoseIterator::operator++(int)
	{
		return PostIncement(*this);
	}

}
