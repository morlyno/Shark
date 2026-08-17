#pragma once

#include "Shark/NodeGraph/Nodes/DebugNodes.h"
#include "Shark/NodeGraph/Nodes/EntityNodes.h"
#include "Shark/NodeGraph/Nodes/MathNodes.h"
#include "Shark/NodeGraph/Nodes/TriggerNodes.h"

#define CREATE_ENTRY(_Node) { ::Shark::NodeGraph::NodeType<_Node>::Class, [](UUID id, NodeContext* context) -> ProcessNode* { return new _Node(id, context); } }
#define NODEGRAPH_CORE_PROCESS_ALLOCATORS						\
	CREATE_ENTRY(::Shark::NodeGraph::Nodes::Test),				\
	CREATE_ENTRY(::Shark::NodeGraph::Nodes::EntityTransform),	\
	CREATE_ENTRY(::Shark::NodeGraph::Nodes::Add<int>),			\
	CREATE_ENTRY(::Shark::NodeGraph::Nodes::Add<float>),		\
	CREATE_ENTRY(::Shark::NodeGraph::Nodes::Multiply<int>),		\
	CREATE_ENTRY(::Shark::NodeGraph::Nodes::Multiply<float>),	\
	CREATE_ENTRY(::Shark::NodeGraph::Nodes::Get),				\
	CREATE_ENTRY(::Shark::NodeGraph::Nodes::Random<int>),		\
	CREATE_ENTRY(::Shark::NodeGraph::Nodes::Random<float>),		\
	CREATE_ENTRY(::Shark::NodeGraph::Nodes::BoolTrigger)
#undef CREATE_ENTRY
