#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Entity.h"
#include "Shark/NodeGraph/ProcessNode.h"
#include "Shark/NodeGraph/PinTypes.h"


namespace Shark::NodeGraph {

	namespace Nodes{

		struct EntityTransform : public ProcessNode
		{
			Types::EntityID* in_Entity = nullptr;

			glm::vec3 Translation;

			EntityTransform(UUID id, NodeContext* context);

			virtual void Initialize(NodeContext* context) override;
			virtual void Process(float ts) override;

		private:
			Entity m_Entity;

		};

	}

	REFLECT_NODE(
		Nodes::EntityTransform,
		REFLECT_INPUTS(&Nodes::EntityTransform::in_Entity),
		REFLECT_OUTPUTS(&Nodes::EntityTransform::Translation)
	);

}
