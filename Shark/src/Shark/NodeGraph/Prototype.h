#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/Identifier.h"

#include <choc/containers/choc_Value.h>

namespace Shark::NodeGraph {

	class Prototype
	{
	public:
		struct Node
		{
			struct Endpoint
			{
				Identifier ID;
				choc::value::Value Value;
			};

			UUID ID;
			Identifier TypeID;

			std::vector<Endpoint> DefaultValues;
		};

		struct Connection
		{
			enum class Type
			{
				Stream,
				Event,
				InputStream,
				OutputStream
			};

			struct Endpoint
			{
				UUID Node;
				Identifier ID;
			};

			Type ConnectionType = Type::Stream;

			Endpoint Start;
			Endpoint End;
		};

		struct Endpoint
		{
			Identifier ID;
			choc::value::Value Value;
		};

		UUID ID;
		std::vector<Node> Nodes;
		std::vector<Connection> Connections;

		std::vector<Endpoint> Inputs;
		std::vector<Endpoint> Outputs;
	};

}
