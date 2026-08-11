#pragma once

#include "Shark/Core/Reflection.h"
#include "Shark/NodeGraph/ProcessNode.h"

#include "NodeGraph/EditorNodes.h"

#include <choc/text/choc_StringUtilities.h>

namespace Shark::NodeGraph::Editor {

	template<typename TFactory>
	struct FactoryHelper
	{
		template<typename TProcNode>
		static void ConstructNode(TFactory* factory, const NodeSettings& settings, std::string_view category, Node& outNode)
		{
			using N = NodeType<TProcNode>;
			auto id = UUID::Generate();

			outNode.ID = id.Value();
			outNode.Name = choc::text::replace(N::Inputs::Class, "<", " (", ">", ")");
			outNode.Category = category;
			outNode.Settings = settings;
			outNode.Inputs = ConstructPinList<TProcNode, typename N::Inputs>(factory, id);
			outNode.Outputs = ConstructPinList<TProcNode, typename N::Outputs>(factory, id);
		}

		template<typename TProcNode, typename TList>
		static auto ConstructPinList(TFactory* factory, UUID nodeID)
		{
			using N = NodeType<TProcNode>;

			constexpr bool isInput = std::is_same_v<TList, typename N::Inputs>;

			std::vector<Editor::Pin> pins;

			TList::ForEachTypeIndexed([&]<typename TInput>(size_t index)
			{
				std::string_view name = TList::Members[index];
				if (name.starts_with("in_"))
					name.remove_prefix(3);

				Pin& newPin = pins.emplace_back();
				InitializePin<TList, TInput>(factory, newPin, name);
				newPin.ID = UUID::Generate().Value();
				newPin.NodeID = nodeID.Value();
				newPin.Name = name;
				newPin.Kind = isInput ? ax::NodeEditor::PinKind::Input : ax::NodeEditor::PinKind::Output;
				newPin.Identifier = name;
			});

			return pins;
		}

		template<typename TList, typename TMemberPtr>
		static bool InitializePin(TFactory* factory, Pin& pin, std::string_view memberName)
		{
			using TMember = Reflection::member_return_type<TMemberPtr>;
			using TMemberRaw = std::remove_pointer_t<TMember>;

			std::optional<int> pinType = GetPinTypeFromMember<TMemberPtr>();

			if (pinType || (pinType = factory->GetPinTypeOverride(TList::Class, memberName)))
				return factory->InitializePin(pin, *pinType);

			return factory->InitializePin(pin, typeid(TMemberRaw));
		}

		template<typename TMemberPtr>
		static std::optional<int> GetPinTypeFromMember()
		{
			using TMember = Reflection::member_return_type<TMemberPtr>;
			using TMemberRaw = std::remove_pointer_t<TMember>;

			if constexpr (std::is_member_function_pointer_v<TMemberPtr> || std::is_same_v<TMemberRaw, ProcessNode::OutputEvent>)
				return 0; // Flow is hard coded to be 0

			// add any global type specific overrides here that can not be detected with typeid/type_info

			return std::nullopt;
		}


		template<typename TProcNode>
		static std::pair<std::string, FactoryEntry> CreateRegistryEntry(TFactory* factory, const NodeSettings& settings = {})
		{
			return {
				choc::text::replace(NodeType<TProcNode>::Inputs::Class, "<", " (", ">", ")"),
				{
					std::bind_front(FactoryHelper<TFactory>::ConstructNode<TProcNode>, factory, settings),
					[](UUID id, NodeContext* context) -> ProcessNode* { return new TProcNode(id, context); }
				}
			};
		}

	};
}
