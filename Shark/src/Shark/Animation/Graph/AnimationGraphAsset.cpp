#include "skpch.h"
#include "AnimationGraphAsset.h"

#include "Shark/Animation/Skeleton.h"
#include "Shark/Animation/Graph/AnimationGraph.h"
#include "Shark/Animation/Graph/AnimationFactory.h"
#include "Shark/Animation/Graph/AnimationNodeContext.h"

#include "Shark/NodeGraph/Prototype.h"

namespace Shark {

	AnimationGraphAsset::AnimationGraphAsset()
	{

	}

	AnimationGraphAsset::~AnimationGraphAsset()
	{

	}

	Ref<NodeGraph::AnimationGraph> AnimationGraphAsset::CreateGraph() const
	{
		auto graph = Ref<NodeGraph::AnimationGraph>::Create(Prototype->ID, m_SkeletonMesh);
		auto skeleton = graph->GetSkeleton();

		NodeGraph::AnimationNodeContext nodeContext;
		nodeContext.Skeleton  = skeleton;
		nodeContext.BoneCount = static_cast<uint32_t>(skeleton->GetBoneCount());

		NodeGraph::Factory factory;

		for (auto& node : Prototype->Nodes)
		{
			NodeGraph::ProcessNode* process = factory.AllocateProcess(node.TypeID, node.ID, &nodeContext);
			
			for (auto& endpoint : node.DefaultValues)
				graph->AddConnection(endpoint.Value, process->GetInput(endpoint.ID));

			graph->AddNode(process);
		}

		for (auto& input : Prototype->Inputs)
		{
			graph->AddGraphInput(input.ID, input.Value);
		}

		for (auto& output : Prototype->Outputs)
		{
			graph->AddGraphOutput(output.ID, output.Value);
		}

		for (auto& connection : Prototype->Connections)
		{
			switch (connection.ConnectionType)
			{
				case NodeGraph::Prototype::Connection::Type::Stream:
				{
					graph->ConnectStream(connection.Start.Node, connection.Start.ID,
										 connection.End.Node, connection.End.ID);
					break;
				}
				case NodeGraph::Prototype::Connection::Type::Event:
				{
					graph->ConnectEvent(connection.Start.Node, connection.Start.ID,
										connection.End.Node, connection.End.ID);
					break;
				}
				case NodeGraph::Prototype::Connection::Type::InputStream:
				{
					graph->ConnectInput(connection.End.Node, connection.End.ID, connection.Start.ID);
					break;
				}
				case NodeGraph::Prototype::Connection::Type::OutputStream:
				{
					graph->ConnectOutput(connection.Start.Node, connection.Start.ID, connection.End.ID);
					break;
				}
			}

		}

		return graph;
	}

}
