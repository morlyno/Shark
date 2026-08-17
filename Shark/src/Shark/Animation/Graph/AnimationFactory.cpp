#include "skpch.h"
#include "AnimationFactory.h"

#include "Shark/NodeGraph/ProcessNode.h"
#include "Shark/NodeGraph/Nodes/AllNodes.h"
#include "Shark/Animation/Graph/Nodes/AnimationNodes.h"
#include "Shark/Animation/Graph/AnimationNodeContext.h"

namespace Shark::NodeGraph {

#define CREATE_ENTRY(_Node) { NodeType<_Node>::Inputs::Class, [](UUID id, AnimationNodeContext* context) -> ProcessNode* { return new _Node(id, context); } }

	Factory::Factory()
	{
		m_Registry = {
			CREATE_ENTRY(Nodes::AnimationPlayer),
			NODEGRAPH_CORE_PROCESS_ALLOCATORS
		};
	}

	ProcessNode* Factory::AllocateProcess(Identifier typeID, UUID id, AnimationNodeContext* context)
	{
		if (m_Registry.contains(typeID))
			return m_Registry.at(typeID)(id, context);
		return nullptr;
	}

}
