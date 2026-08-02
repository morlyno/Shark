#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Entity.h"

#include "NodeGraph/ProcessNode.h"
#include "NodeGraph/PinTypes.h"


namespace Shark::NodeGraph {

	namespace Nodes{

		struct EntityTransform : public ProcessNode
		{
			Types::EntityID* Entity = nullptr;

			glm::vec3 Translation;

			EntityTransform(UUID id, NodeContext* context);

			virtual void Initialize(NodeContext* context) override;
			virtual void Process(float ts) override;

		private:
			Shark::Entity m_Entity;

		};

	}

}

REFLECT_NODE(
	Shark::NodeGraph::Nodes::EntityTransform,
	REFLECT_INPUTS(&Shark::NodeGraph::Nodes::EntityTransform::Entity),
	REFLECT_OUTPUTS(&Shark::NodeGraph::Nodes::EntityTransform::Translation)
);
