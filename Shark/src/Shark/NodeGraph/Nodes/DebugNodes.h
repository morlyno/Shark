#pragma once

#include "Shark/Core/Base.h"
#include "Shark/NodeGraph/ProcessNode.h"

namespace Shark::NodeGraph {

	namespace Nodes {

		struct Test : public Details::TypedNode<Test>
		{
		public:
			using Details::TypedNode<Test>::TypedNode;
			virtual void Process(float ts) override
			{
			}

			void Print()
			{
				SK_CORE_DEBUG("Test");
			}

		};

	}

	REFLECT_NODE(
		Nodes::Test,
		REFLECT_INPUTS(&Nodes::Test::Print),
		REFLECT_OUTPUTS()
	);

}
