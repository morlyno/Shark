#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Asset/AssetTypes.h"

namespace Shark {
	class Scene;
	class Entity;
	class MeshSource;

	class Skeleton;
	class Animation;
	struct Pose;
}

namespace Shark {

	class AnimationEngine
	{
	public:
		class PoseIterator;

	public:
		AnimationEngine() = default;
		AnimationEngine(Ref<Scene> scene);

		void SetCurrentScene(Ref<Scene> scene);
		void RegisterEntity(const Entity& entity);
		void Transition(UUID targetEntityID, AssetHandle animationHandle, float duration, bool loop);
		void Update(TimeStep ts);

		PoseIterator GetPoses();
		const Pose* GetPose(UUID entityID) const;
		void SetSamplePosition(UUID entityID, float position);

		void OnAssetReloaded(AssetHandle handle);

	private:
		struct AnimationEntry;
		void UpdateEntries();
		void UpdateAnimation(TimeStep ts, AnimationEntry& entry);

		std::pair<UUID, Pose*> GetEntityAndPose(size_t index);
		void AdvanceIterator(PoseIterator& iterator);

	private:
		struct AnimationEntry
		{
			UUID EntityID;
			AssetHandle SkeletonMesh;
			AssetHandle ActiveAnimation;
			const Animation* Animation = nullptr;
			const Skeleton* Skeleton = nullptr;
			Scope<Pose> Pose = nullptr;
			float SamplePosition = 0.0f;

			bool Update = false;
			bool Loop = false;
			bool IsTransition = false;
		};

		struct TransitionEntry
		{
			float Blend;
			float Duration;

			uint32_t Target;
			uint32_t Transition;
		};

		// Use raw pinter because a ref will cause a cyclic dependency with scene because this os owned by Scene
		Scene* m_CurrentScene;
		std::vector<AnimationEntry> m_RegisteredAnimations;
		std::vector<TransitionEntry> m_Transitions;
	};

	class AnimationEngine::PoseIterator
	{
	public:
		PoseIterator() = default;
		PoseIterator(AnimationEngine& engine)
			: m_Engine(&engine), m_Index(0), m_Entry(m_Engine->GetEntityAndPose(0))
		{}

		std::pair<UUID, Pose*>& operator*() { return m_Entry; }
		std::pair<UUID, Pose*>* operator->() { return &m_Entry; }

		bool operator==(const PoseIterator& other) const { return m_Index == other.m_Index || IsAtEnd() && other.IsAtEnd(); }
		bool operator!=(const PoseIterator& other) const { return !(*this == other); }

		PoseIterator& operator++();
		PoseIterator operator++(int);

	private:
		bool IsAtEnd() const { return !m_Engine || m_Index >= m_Engine->m_RegisteredAnimations.size(); }

	private:
		AnimationEngine* m_Engine = nullptr;

		size_t m_Index = ~0;
		std::pair<UUID, Pose*> m_Entry;

		friend class AnimationEngine;
	};

	inline AnimationEngine::PoseIterator begin(AnimationEngine::PoseIterator iter) { return iter; }
	inline AnimationEngine::PoseIterator end(AnimationEngine::PoseIterator end)    { return {};   }

}
