#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/Reflection.h"
#include "NodeGraph/Identifier.h"
#include "Shark/Utils/String.h"

namespace Shark {

	struct Node
	{
		using InputType = float*;
		using OutputType = float;

		void AddInput(Identifier id, InputType& input)
		{
			Inputs.emplace(id, &input);
		}

		void AddOutput(Identifier id, OutputType& output)
		{
			Outputs.emplace(id, &output);
		}

		InputType* GetInput(Identifier id) const
		{
			if (Inputs.contains(id))
				return Inputs.at(id);
			return nullptr;
		}

		OutputType* GetOutput(Identifier id) const
		{
			if (Outputs.contains(id))
				return Outputs.at(id);
			return nullptr;
		}

		virtual void Process() = 0;

		Node(UUID id)
			: ID(id)
		{}

		UUID ID;
		std::unordered_map<Identifier, InputType*> Inputs;
		std::unordered_map<Identifier, OutputType*> Outputs;
	};

	namespace Nodes {

		struct Add : public Node
		{
			float* Value1 = nullptr;
			float* Value2 = nullptr;

			float Result;

			Add(UUID id);

			virtual void Process() override
			{
				Result = *Value1 + *Value2;
			}

			struct IDs
			{
				static constexpr Identifier Input1 = "Value1";
				static constexpr Identifier Input2 = "Value2";
				static constexpr Identifier Output = "Result";
			};

		};

		struct Multiply : public Node
		{
			float* Value1 = nullptr;
			float* Value2 = nullptr;

			float Result;

			Multiply(UUID id);

			virtual void Process() override
			{
				Result = *Value1 * *Value2;
			}
		};

	}

	struct ReflectionInputTag {};
	struct ReflectionOutputTag {};

	template<typename T>
	struct NodeType;

#define REFLECT_INPUTS(...) __VA_ARGS__
#define REFLECT_OUTPUTS(...) __VA_ARGS__

#define REFLECT_NODE(_node, _inputs, _outputs)						  \
	REFLECT_TYPE_TAGGED(_node, ReflectionInputTag, _inputs);		  \
	REFLECT_TYPE_TAGGED(_node, ReflectionOutputTag, _outputs);		  \
																	  \
	template<>														  \
	struct NodeType<_node>											  \
	{																  \
		using Inputs = Reflection::Type<_node, ReflectionInputTag>;	  \
		using Outputs = Reflection::Type<_node, ReflectionOutputTag>; \
	};

	REFLECT_NODE(
		Nodes::Add,
		REFLECT_INPUTS(&Nodes::Add::Value1,
					   &Nodes::Add::Value2),
		REFLECT_OUTPUTS(&Nodes::Add::Result)
	);

	REFLECT_NODE(
		Nodes::Multiply,
		REFLECT_INPUTS(&Nodes::Multiply::Value1,
					   &Nodes::Multiply::Value2),
		REFLECT_OUTPUTS(&Nodes::Multiply::Result)
	);

}
