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

	namespace NodeGraph {
		class AnimationGraph;
	}
}

namespace Shark {

	class AnimationEngine
	{
	public:
		class PoseIterator;
		class PoseEndIterator;

	public:
		AnimationEngine();
		AnimationEngine(Ref<Scene> scene);
		~AnimationEngine();

		void SetCurrentScene(Ref<Scene> scene);
		void RegisterEntity(const Entity& entity);
		void Transition(UUID targetEntityID, AssetHandle animationHandle, float duration, bool loop);
		void Update(TimeStep ts);

		bool Registered(UUID entityID) const;
		bool RegisteredAnimation(UUID entityID) const;
		bool RegisteredGraph(UUID entityID) const;

		PoseIterator GetPoses();
		const Pose* GetPose(UUID entityID) const;
		Ref<NodeGraph::AnimationGraph> GetGraph(UUID entityID) const;
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

		struct GraphEntry
		{
			UUID EntityID;
			AssetHandle Animation;
			Ref<NodeGraph::AnimationGraph> Graph;
		};

		// Use raw pinter because a ref will cause a cyclic dependency with scene because this os owned by Scene
		Scene* m_CurrentScene;
		std::vector<AnimationEntry> m_RegisteredAnimations;
		std::vector<TransitionEntry> m_Transitions;

		std::vector<GraphEntry> m_RegisteredGraphs;
	};

	class AnimationEngine::PoseIterator
	{
	public:
		PoseIterator() = default;
		PoseIterator(AnimationEngine& engine)
			: m_Engine(&engine), m_Index(0), m_Entry(m_Engine->GetEntityAndPose(0))
		{}

		std::pair<UUID, const Pose*>& operator*() { return m_Entry; }
		std::pair<UUID, const Pose*>* operator->() { return &m_Entry; }

		bool operator==(const PoseEndIterator& other) const { return IsAtEnd(); }
		bool operator!=(const PoseEndIterator& other) const { return !(*this == other); }

		PoseIterator& operator++();
		PoseIterator operator++(int);

	private:
		bool IsAtEnd() const { return !m_Engine || (m_Index == ~0 && m_GraphIndex == ~0); }

	private:
		AnimationEngine* m_Engine = nullptr;

		size_t m_Index = ~0;
		size_t m_GraphIndex = ~0;
		std::pair<UUID, const Pose*> m_Entry;

		friend class AnimationEngine;
	};

	class AnimationEngine::PoseEndIterator
	{
	public:
	};

	inline AnimationEngine::PoseIterator begin(AnimationEngine::PoseIterator iter) { return iter; }
	inline AnimationEngine::PoseEndIterator end(AnimationEngine::PoseIterator end) { return {};   }

}
