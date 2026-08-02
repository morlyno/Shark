#pragma once

#include "Shark/Core/Base.h"
#include "NodeGraph/ProcessNode.h"
#include "NodeGraph/NodeContext.h"

namespace Shark::NodeGraph {

	namespace Nodes {

		struct BoolTrigger : public ProcessNode
		{
			const bool* Value          = nullptr;
			const bool* TriggerIfTrue  = nullptr;
			const bool* TriggerIfFalse = nullptr;

			// replace with event
			bool Triggered;
			OutputEvent Trigger;

		public:
			BoolTrigger(UUID id, NodeContext* context)
				: ProcessNode(id)
			{
				Details::RegisterVariables(this);
			}

			virtual void Initialize(NodeContext* context) override
			{
				Details::InitializeInputs(this);
			}

			virtual void Process(float ts) override
			{
				if (Triggered = *Value && *TriggerIfTrue || !*Value && *TriggerIfFalse)
					Trigger();
			}

		};

	}

}

REFLECT_NODE(Shark::NodeGraph::Nodes::BoolTrigger,
			 REFLECT_INPUTS(&Shark::NodeGraph::Nodes::BoolTrigger::Value,
							&Shark::NodeGraph::Nodes::BoolTrigger::TriggerIfTrue,
							&Shark::NodeGraph::Nodes::BoolTrigger::TriggerIfFalse),
			 REFLECT_OUTPUTS(&Shark::NodeGraph::Nodes::BoolTrigger::Triggered,
							 &Shark::NodeGraph::Nodes::BoolTrigger::Trigger)
);
