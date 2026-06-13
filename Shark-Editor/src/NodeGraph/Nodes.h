#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/Reflection.h"
#include "Shark/Utils/String.h"
#include "NodeGraph/Identifier.h"

#include <choc/containers/choc_Value.h>

namespace Shark {

	struct Node
	{
		void AddInput(Identifier id, choc::value::Type type)
		{
			Inputs.try_emplace(id, choc::value::ValueView(type, nullptr, nullptr));
		}

		template<typename T>
		void AddOutput(Identifier id, T& output)
		{
			Outputs.emplace(id, choc::value::ValueView(choc::value::Type::createPrimitive<T>(), &output, nullptr));
		}

		choc::value::ValueView& GetInput(Identifier id)  { return Inputs.at(id); }
		choc::value::ValueView& GetOutput(Identifier id) { return Outputs.at(id); }

		virtual void Initialize() = 0;
		virtual void Process() = 0;

		Node(UUID id)
			: ID(id)
		{}

		UUID ID;
		std::unordered_map<Identifier, choc::value::ValueView> Inputs;
		std::unordered_map<Identifier, choc::value::ValueView> Outputs;
	};

	namespace Details {

		template<typename T>
		static void RegisterVariables(T* node);

		template<typename T>
		static void InitializeInputs(T* node);

		template<typename T>
		struct TypedNode : public Node
		{
			TypedNode(UUID id)
				: Node(id)
			{
				RegisterVariables(static_cast<T*>(this));
			}

			virtual void Initialize() override
			{
				InitializeInputs(static_cast<T*>(this));
			}

		protected:
			using Base = TypedNode<T>;
		};

	}

	namespace Nodes {

		template<typename T>
		struct Add;

		template<>
		struct Add<float> : public Details::TypedNode<Add<float>>
		{
			float* Value1 = nullptr;
			float* Value2 = nullptr;

			float Result;

			using Base::Base;
			virtual void Process() override
			{
				Result = *Value1 + *Value2;
			}
		};

		template<>
		struct Add<int> : public Details::TypedNode<Add<int>>
		{
			int* Value1 = nullptr;
			int* Value2 = nullptr;

			int Result;

			using Base::Base;
			virtual void Process() override
			{
				Result = *Value1 + *Value2;
			}
		};

		template<typename T>
		struct Multiply;

		template<>
		struct Multiply<float> : public Details::TypedNode<Multiply<float>>
		{
			float* Value1 = nullptr;
			float* Value2 = nullptr;

			float Result;

			using Base::Base;
			virtual void Process() override
			{
				Result = *Value1 * *Value2;
			}
		};

		template<>
		struct Multiply<int> : public Details::TypedNode<Multiply<int>>
		{
			int* Value1 = nullptr;
			int* Value2 = nullptr;

			int Result;

			using Base::Base;
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
		Nodes::Add<float>,
		REFLECT_INPUTS(&Nodes::Add<float>::Value1,
					   &Nodes::Add<float>::Value2),
		REFLECT_OUTPUTS(&Nodes::Add<float>::Result)
	);

	REFLECT_NODE(
		Nodes::Add<int>,
		REFLECT_INPUTS(&Nodes::Add<int>::Value1,
					   &Nodes::Add<int>::Value2),
		REFLECT_OUTPUTS(&Nodes::Add<int>::Result)
	);

	REFLECT_NODE(
		Nodes::Multiply<float>,
		REFLECT_INPUTS(&Nodes::Multiply<float>::Value1,
					   &Nodes::Multiply<float>::Value2),
		REFLECT_OUTPUTS(&Nodes::Multiply<float>::Result)
	);

	REFLECT_NODE(
		Nodes::Multiply<int>,
		REFLECT_INPUTS(&Nodes::Multiply<int>::Value1,
					   &Nodes::Multiply<int>::Value2),
		REFLECT_OUTPUTS(&Nodes::Multiply<int>::Result)
	);

}

namespace Shark::Details {

	template<typename T>
	static void RegisterVariables(T* node)
	{
		using N = NodeType<T>;

		N::Inputs::Apply([&node](const auto&... members)
		{
			auto unpack = [&node, nodeIndex = 0](auto memberPtr) mutable
			{
				const auto createType = []<typename TMember>(TMember & member)
				{
					return choc::value::Type::createPrimitive<std::remove_pointer_t<TMember>>();
				};

				std::string_view name = N::Inputs::Members[nodeIndex++];
				node->AddInput(name, createType(node->*memberPtr));
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

	template<typename T>
	static void InitializeInput(Node* node, T& value, std::string_view name)
	{
		value = static_cast<T>(node->GetInput(name).getRawData());
	}

	template<typename T>
	static void InitializeInputs(T* node)
	{
		using N = NodeType<T>;

		N::Inputs::Apply([&node](const auto&... members)
		{
			auto unpack = [&node, nodeIndex = 0](auto memberPtr) mutable
			{
				using MemberType = decltype(node->*memberPtr);

				std::string_view name = N::Inputs::Members[nodeIndex++];
				InitializeInput(node, node->*memberPtr, name);
			};

			(unpack(members), ...);
		});
	}

}
