#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "NodeGraph/Identifier.h"
#include "Shark/Utils/std.h"

#include <random>

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

			Add(UUID id)
				: Node(id)
			{
				AddInput(IDs::Input1, Value1);
				AddInput(IDs::Input2, Value2);
				AddOutput(IDs::Output, Result);
			}

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

	}

}
