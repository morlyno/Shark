#include "Nodes.h"

namespace Shark {

	template<typename T>
	static void RegisterVariables(T* node)
	{
		using N = NodeType<T>;

		N::Inputs::Apply([&node](const auto&... members)
		{
			auto unpack = [&node, nodeIndex = 0](auto memberPtr) mutable
			{
				std::string_view name = N::Inputs::Members[nodeIndex++];
				node->AddInput(name, node->*memberPtr);
			};

			(unpack(members), ...);
		});

		N::Outputs::Apply([&node](const auto&... members)
		{
			auto unpack = [&node, nodeIndex = 0](auto memberPtr) mutable
			{
				std::string_view name = N::Outputs::Members[nodeIndex++];
				node->AddOutput(name, node->*memberPtr);
			};

			(unpack(members), ...);
		});

	}

	Nodes::Add::Add(UUID id)
		: Node(id)
	{
		RegisterVariables(this);
	}

	Nodes::Multiply::Multiply(UUID id)
		: Node(id)
	{
		RegisterVariables(this);
	}

}
