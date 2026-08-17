#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/Identifier.h"

namespace Shark::NodeGraph {
	struct ProcessNode;
	struct AnimationNodeContext;
}

namespace Shark::NodeGraph {

	class Factory
	{
	public:
		Factory();

		ProcessNode* AllocateProcess(Identifier typeID, UUID id, AnimationNodeContext* context);

	private:
		std::unordered_map<Identifier, ProcessNode* (*)(UUID, AnimationNodeContext*)> m_Registry;
	};

}
