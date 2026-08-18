#include "skpch.h"
#include "AnimationGraph.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/MeshSource.h"
#include "Shark/Animation/Skeleton.h"

#include "Shark/Animation/Graph/PinTypes.h"
#include "Shark/Animation/Graph/AnimationNodeContext.h"

#include "Shark/Utils/Utilities.h"

namespace Shark::NodeGraph {

	AnimationGraph::AnimationGraph(UUID prototypeID, AssetHandle skeletonMesh)
		: ProcessNode(UUID::Make(skeletonMesh)), m_PrototypeID(prototypeID)
	{
		SetSkeletonMesh(skeletonMesh);
	}

	AnimationGraph::~AnimationGraph()
	{

	}

	void AnimationGraph::InitializeGraph()
	{
		NodeContext context;
		Initialize(&context);
	}

	void AnimationGraph::Initialize(NodeContext* context)
	{
		const auto init = [this](AnimationNodeContext* context)
		{
			for (auto& node : m_Nodes)
				node->Initialize(context);
		};

		if (context->Domain == AnimationGraphDomain)
		{
			init(static_cast<AnimationNodeContext*>(context));
			return;
		}

		AnimationNodeContext animationContext{};
		static_cast<NodeContext&>(animationContext) = *context;
		animationContext.Domain = AnimationGraphDomain;
		animationContext.Skeleton = m_Skeleton;
		animationContext.BoneCount = static_cast<uint32_t>(m_Skeleton->GetBoneCount());

		init(&animationContext);
	}

	void AnimationGraph::Process(float ts)
	{
		for (auto& node : m_Nodes)
			node->Process(ts);
	}

	void AnimationGraph::SetSkeletonMesh(AssetHandle skeletonMesh)
	{
		m_SkeletonMesh = skeletonMesh;
		if (auto mesh = AssetManager::GetAsset<MeshSource>(skeletonMesh))
			m_Skeleton = &mesh->GetSkeleton();
	}

	AssetHandle AnimationGraph::GetSkeletonMesh() const
	{
		return m_SkeletonMesh;
	}

	const Skeleton* AnimationGraph::GetSkeleton() const
	{
		return m_Skeleton;
	}

	const Pose* AnimationGraph::GetPose()
	{
		return static_cast<Pose*>(GetOutput("Pose").getRawData());
	}

	void AnimationGraph::AddNode(ProcessNode* node)
	{
		m_Nodes.emplace_back(node);
	}

	void AnimationGraph::AddNode(Scope<ProcessNode> node)
	{
		m_Nodes.emplace_back(std::move(node));
	}

	bool AnimationGraph::AddConnection(choc::value::ValueView output, choc::value::ValueView& input)
	{
		input = output;
		return true;
	}

	bool AnimationGraph::AddConnection(OutputEvent& output, const InputEvent& target)
	{
		output.AddTarget(target);
		return true;
	}

	bool AnimationGraph::ConnectEvent(UUID startNodeID, Identifier outputID, UUID endNodeID, Identifier inputID)
	{
		ProcessNode* startNode = find_as_ptr(m_Nodes, startNodeID, &ProcessNode::ID);
		ProcessNode* endNode = find_as_ptr(m_Nodes, endNodeID, &ProcessNode::ID);

		if (!startNode || !endNode || !startNode->IsOutputEvent(outputID) || !endNode->IsInputEvent(inputID))
			return false;

		return AddConnection(startNode->GetOutputEvent(outputID), endNode->GetInputEvent(inputID));
	}

	bool AnimationGraph::ConnectStream(UUID startNodeID, Identifier outputID, UUID endNodeID, Identifier inputID)
	{
		ProcessNode* startNode = find_as_ptr(m_Nodes, startNodeID, &ProcessNode::ID);
		ProcessNode* endNode = find_as_ptr(m_Nodes, endNodeID, &ProcessNode::ID);

		if (!startNode || !endNode || !startNode->IsOutput(outputID) || !endNode->IsInput(inputID))
			return false;

		return AddConnection(startNode->GetOutput(outputID), endNode->GetInput(inputID));
	}

	bool AnimationGraph::ConnectInput(UUID nodeID, Identifier id, Identifier inputID)
	{
		ProcessNode* endNode = find_as_ptr(m_Nodes, nodeID, &ProcessNode::ID);

		if (!endNode || !endNode->IsInput(id))
			return false;

		return AddConnection(GetInput(inputID), endNode->GetInput(id));
	}

	bool AnimationGraph::ConnectOutput(UUID nodeID, Identifier id, Identifier outputID)
	{
		ProcessNode* startNode = find_as_ptr(m_Nodes, nodeID, &ProcessNode::ID);

		if (!startNode || !startNode->IsOutput(id))
			return false;

		return AddConnection(startNode->GetOutput(id), GetOutput(id));
	}

	void AnimationGraph::AddGraphInput(Identifier id, choc::value::Value value)
	{
		AddInput(id, m_GraphInputs.emplace_back(std::move(value)));
	}

	void AnimationGraph::AddGraphOutput(Identifier id, choc::value::ValueView value)
	{
		AddOutput(id, value);
	}

}
