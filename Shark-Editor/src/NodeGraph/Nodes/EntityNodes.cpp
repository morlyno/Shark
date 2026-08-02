#include "skpch.h"
#include "EntityNodes.h"

namespace Shark::NodeGraph {

	Nodes::EntityTransform::EntityTransform(UUID id, NodeContext* context)
		: ProcessNode(id)
	{
		Details::RegisterVariables(this);
	}

	void Nodes::EntityTransform::Initialize(NodeContext* context)
	{
		Details::InitializeInputs(this);

		// #Investigate #nodegraph entity needs to be reinitialized when it is an input to the graph.
		//                         should this happen in Process or should the graph call Initialize
		//                         every frame for all nodes with inputs (maybe only those who actually need it)

		m_Entity = context->GetActiveScene()->TryGetEntityByUUID(UUID::Make(*Entity));
		SK_CORE_VERIFY(m_Entity);
	}

	void Nodes::EntityTransform::Process(float ts)
	{
		auto& transform = m_Entity.Transform();
		Translation = transform.Translation;
	}

}
