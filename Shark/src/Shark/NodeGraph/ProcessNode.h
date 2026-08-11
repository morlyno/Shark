#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/Reflection.h"
#include "Shark/Core/Identifier.h"
#include "Shark/NodeGraph/PinTypes.h"
#include "Shark/NodeGraph/NodeContext.h"

#include <choc/containers/choc_Value.h>

namespace Shark::NodeGraph {

	struct ProcessNode
	{
		struct InputEvent
		{
			InputEvent(std::function<void()> event)
				: Event(std::move(event)) {}

			void operator()() const
			{
				Event();
			}

			std::function<void()> Event; // Target/Member function of a node
		};

		struct OutputEvent
		{
			void operator()() const
			{
				for (auto& target : Targets)
					(*target)();
			}

			void AddTarget(InputEvent& target)
			{
				Targets.emplace_back(&target);
			}

			std::vector<InputEvent*> Targets;
		};

		void AddInput(Identifier id, auto& input)                       { Inputs.try_emplace(id); }
		void AddOutput(Identifier id, auto& output)                     { Outputs.emplace(id, choc::value::ValueView(CreateType<decltype(output)>(), &output, nullptr)); }
		void AddOutput(Identifier id, choc::value::Value& output)       { Outputs.emplace(id, output); }
		void AddOutput(Identifier id, choc::value::ValueView output)    { Outputs.emplace(id, output); }

		void AddInputEvent(Identifier id, std::function<void()> target) { InputEvents.emplace(id, std::move(target)); }
		void AddOutputEvent(Identifier id, OutputEvent& output)         { OutputEvents.emplace(id, output); }

		bool                    IsInputEvent(Identifier id) const  { return InputEvents.contains(id); }
		bool                    IsOutputEvent(Identifier id) const { return OutputEvents.contains(id); }
		choc::value::ValueView& GetInput(Identifier id)            { return Inputs.at(id); }
		choc::value::ValueView& GetOutput(Identifier id)           { return Outputs.at(id); }
		InputEvent&             GetInputEvent(Identifier id)       { return InputEvents.at(id); }
		OutputEvent&            GetOutputEvent(Identifier id)      { return OutputEvents.at(id); }

		virtual void Initialize(NodeContext* context) = 0;
		virtual void Process(float ts) = 0;

		ProcessNode(UUID id)
			: ID(id)
		{}

		UUID ID;
		std::unordered_map<Identifier, choc::value::ValueView> Inputs;
		std::unordered_map<Identifier, choc::value::ValueView> Outputs;

		std::unordered_map<Identifier, InputEvent> InputEvents;
		std::unordered_map<Identifier, OutputEvent&> OutputEvents;
	};

	namespace Details {

		template<typename T>
		static void RegisterVariables(T* node);

		template<typename T>
		static void InitializeInputs(T* node);

		template<typename T>
		struct TypedNode : public ProcessNode
		{
			TypedNode(UUID id, NodeContext* context)
				: ProcessNode(id)
			{
				RegisterVariables(static_cast<T*>(this));
			}

			virtual void Initialize(NodeContext* context) override
			{
				InitializeInputs(static_cast<T*>(this));
			}
		};

	}

	struct ReflectionInputTag {};
	struct ReflectionOutputTag {};

	template<typename T>
	struct NodeType;

	REFLECTION_PROXY(NodeReflectionProxy);

}

REFLECTION_PROXY_CONNECT(Shark::NodeGraph::NodeReflectionProxy);

#define REFLECT_INPUTS(...) __VA_ARGS__
#define REFLECT_OUTPUTS(...) __VA_ARGS__
#define REFLECT_DEFAULTS(...) __VA_ARGS__

#define REFLECT_NODE(_node, _inputs, _outputs)																	\
	REFLECT_TYPE_TAGGED_PROXY(NodeReflectionProxy, _node, ::Shark::NodeGraph::ReflectionInputTag, _inputs);		\
	REFLECT_TYPE_TAGGED_PROXY(NodeReflectionProxy, _node, ::Shark::NodeGraph::ReflectionOutputTag, _outputs);	\
																												\
	template<>																									\
	struct ::Shark::NodeGraph::NodeType<_node>																	\
	{																											\
		using Inputs = ::Shark::Reflection::Type<_node, ReflectionInputTag>;									\
		using Outputs = ::Shark::Reflection::Type<_node, ReflectionOutputTag>;									\
	};


namespace Shark::NodeGraph::Details {

	constexpr std::string_view RemovePinPrefix(std::string_view name)
	{
		if (name.starts_with("in_"))
			name.remove_prefix(3);
		if (name.starts_with("out_"))
			name.remove_prefix(4);
		return name;
	}

	template<typename T>
	static void RegisterVariables(T* node)
	{
		using N = NodeType<T>;

		N::Inputs::Apply([&node](const auto&... members)
		{
			auto unpack = [&node, nodeIndex = 0](auto memberPtr) mutable
			{
				std::string_view name = RemovePinPrefix(N::Inputs::Members[nodeIndex++]);

				if constexpr (std::is_member_function_pointer_v<decltype(memberPtr)>)
				{
					node->AddInputEvent(name, [node, memberPtr]() { (node->*memberPtr)(); });
				}
				else
				{
					node->AddInput(name, node->*memberPtr);
				}
			};

			(unpack(members), ...);
		});

		N::Outputs::Apply([&node](const auto&... members)
		{
			auto unpack = [&node, nodeIndex = 0](auto memberPtr) mutable
			{
				std::string_view name = RemovePinPrefix(N::Outputs::Members[nodeIndex++]);
				using TMember = Reflection::member_return_type<decltype(memberPtr)>;

				if constexpr (std::is_same_v<TMember, ProcessNode::OutputEvent>)
				{
					node->AddOutputEvent(name, node->*memberPtr);
				}
				else
				{
					node->AddOutput(name, node->*memberPtr);
				}
			};

			(unpack(members), ...);
		});

	}

	template<typename T>
	static void InitializeInput(ProcessNode* node, T& value, std::string_view name)
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
				std::string_view name = RemovePinPrefix(N::Inputs::Members[nodeIndex++]);
				if constexpr (std::is_member_function_pointer_v<decltype(memberPtr)>)
				{
				}
				else
				{
					InitializeInput(node, node->*memberPtr, name);
				}
			};

			(unpack(members), ...);
		});
	}

}
