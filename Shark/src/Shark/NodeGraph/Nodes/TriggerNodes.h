#pragma once

#include "Shark/Core/Base.h"
#include "Shark/NodeGraph/ProcessNode.h"
#include "Shark/NodeGraph/NodeContext.h"

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

	REFLECT_NODE(
		Nodes::BoolTrigger,
		REFLECT_INPUTS(&Nodes::BoolTrigger::Value,
					   &Nodes::BoolTrigger::TriggerIfTrue,
					   &Nodes::BoolTrigger::TriggerIfFalse),
		REFLECT_OUTPUTS(&Nodes::BoolTrigger::Triggered,
						&Nodes::BoolTrigger::Trigger)
	);

}
