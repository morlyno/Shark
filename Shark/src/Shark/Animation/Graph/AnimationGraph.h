#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/AssetTypes.h"
#include "Shark/Animation/Pose.h"
#include "Shark/NodeGraph/ProcessNode.h"

namespace Shark {
	class Skeleton;
}

namespace Shark::NodeGraph {

	class AnimationGraph : public ProcessNode, public RefCount
	{
	public:
		AnimationGraph(UUID prototypeID, AssetHandle skeletonMesh);
		~AnimationGraph();

		void InitializeGraph();
		virtual void Initialize(NodeContext* context) override;
		virtual void Process(float ts) override;

		void SetSkeletonMesh(AssetHandle skeletonMesh);
		AssetHandle GetSkeletonMesh() const;
		const Skeleton* GetSkeleton() const;

		const Pose* GetPose();
		UUID GetPrototypeID() const { return m_PrototypeID; }

	public:
		void AddNode(ProcessNode* node);
		void AddNode(Scope<ProcessNode> node);

		bool AddConnection(choc::value::ValueView output, choc::value::ValueView& input);
		bool AddConnection(OutputEvent& output, const InputEvent& target);
		bool ConnectEvent(UUID startNodeID, Identifier outputID, UUID endNodeID, Identifier inputID);
		bool ConnectStream(UUID startNodeID, Identifier outputID, UUID endNodeID, Identifier inputID);
		bool ConnectInput(UUID nodeID, Identifier id, Identifier inputID);
		bool ConnectOutput(UUID nodeID, Identifier id, Identifier outputID);

		void AddGraphInput(Identifier id, choc::value::Value value);
		void AddGraphOutput(Identifier id, choc::value::ValueView value);

	private:
		UUID m_PrototypeID;
		AssetHandle m_SkeletonMesh;
		const Skeleton* m_Skeleton = nullptr;

		std::vector<Scope<ProcessNode>> m_Nodes;
		std::vector<choc::value::Value> m_GraphInputs;

	};

}
