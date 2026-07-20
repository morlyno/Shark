#pragma once

#include "Shark/Core/UUID.h"
#include "Shark/Core/Reflection.h"
#include "NodeGraph/EditorNodes.h"
#include "NodeGraph/ProcessNode.h"
#include "NodeGraph/PinTypes.h"

#include <imgui_node_editor.h>
#include <choc/containers/choc_Value.h>
#include <choc/text/choc_StringUtilities.h>

#include <tuple>

namespace Shark::NodeGraph::Editor {

	struct CoreTypes
	{
		enum EPinType : int
		{
			Flow,
			Bool,
			Int,
			Float,
			Vec3,
			EntityID,
		};

		template<int TPinType, typename TValueType, ImColor TColor = IM_COL32_WHITE>
		struct PinDescriptor
		{
			static constexpr auto PinType = TPinType;
			static constexpr auto Color = TColor;
			using value_type = TValueType;
		};

		using PinTypes = std::tuple<
			PinDescriptor<EPinType::Flow,     Types::Flow,     ImColor(255, 255, 255)>,
			PinDescriptor<EPinType::Bool,     bool,            ImColor(220,  48,  48)>,
			PinDescriptor<EPinType::Int,      int,             ImColor( 68, 201, 156)>,
			PinDescriptor<EPinType::Float,    float,           ImColor(147, 226,  74)>,
			PinDescriptor<EPinType::Vec3,     glm::vec3,       ImColor(147, 226,  74)>,
			PinDescriptor<EPinType::EntityID, Types::EntityID, ImColor( 51, 150, 215)>
		>;

		template<typename TMemberPtr>
		static EPinType GetPinTypeFromMember()
		{
			using TMember = Reflection::member_return_type<TMemberPtr>;
			using TMemberRaw = std::remove_pointer_t<TMember>;

			if constexpr (std::is_member_function_pointer_v<TMemberPtr> || std::is_same_v<TMemberPtr, ProcessNode::OutputEvent>)
				return EPinType::Flow;

			EPinType pinType = EPinType::Flow;

			Reflection::ForEach(PinTypes{}, [&pinType]<typename TDesc>()
			{
				if constexpr (std::is_same_v<const TDesc::value_type, const TMemberRaw>)
				{
					pinType = static_cast<EPinType>(TDesc::PinType);
				}
			});

			return pinType;
		}

		static bool InitializePin(Pin& pin, EPinType pinType)
		{
			bool initialized = false;

			Reflection::ForEach(PinTypes{}, [&pin, pinType, &initialized]<typename TDesc>()
			{
				if (TDesc::PinType == pinType)
				{
					pin.SetDesc<TDesc>();
					initialized = true;
				}
			});

			return initialized;
		}

		static bool InitiailizePin(Pin& pin, const choc::value::Type& type)
		{
			if (type.isObjectWithClassName("Flow"))
				return InitializePin(pin, EPinType::Flow);

			if (type.isObjectWithClassName("EntityID"))
				return InitializePin(pin, EPinType::EntityID);

			if (type.isBool())
				return InitializePin(pin, EPinType::Bool);
			if (type.isInt32())
				return InitializePin(pin, EPinType::Int);
			if (type.isFloat32())
				return InitializePin(pin, EPinType::Float);

			SK_CORE_ASSERT(false, "Unknown Type");
			return false;
		}

		static Pin ConstructPin(EPinType pinType)
		{
			Pin pin;

			Reflection::ForEach(PinTypes{}, [&pin, pinType]<typename TDesc>()
			{
				if (TDesc::PinType == pinType)
					pin.SetDesc<TDesc>();
			});

			return pin;
		}

		template<typename TProcNode, typename TList>
		static auto ConstructPinList(UUID nodeID)
		{
			using N = NodeType<TProcNode>;

			constexpr bool isInput = std::is_same_v<TList, typename N::Inputs>;

			std::vector<Editor::Pin> pins;

			TList::ForEachTypeIndexed([&]<typename TInput>(size_t index)
			{
				Pin& newPin = pins.emplace_back();
				InitializePin(newPin, GetPinTypeFromMember<TInput>());
				newPin.ID = UUID::Generate().Value();
				newPin.NodeID = nodeID.Value();
				newPin.Name = TList::Members[index];
				newPin.Kind = isInput ? ax::NodeEditor::PinKind::Input : ax::NodeEditor::PinKind::Output;
				newPin.Identifier = TList::Members[index];
			});

			return pins;
		}

		template<typename TProcNode>
		static void SpawnNode(const NodeSettings& settings, std::string_view category, Node& outNode, UUID id)
		{
			using N = NodeType<TProcNode>;

			outNode.ID = id.Value();
			outNode.Name = choc::text::replace(N::Inputs::Class, "<", " (", ">", ")");
			outNode.Category = category;
			outNode.Settings = settings;
			outNode.Inputs = ConstructPinList<TProcNode, typename N::Inputs>(id);
			outNode.Outputs = ConstructPinList<TProcNode, typename N::Outputs>(id);
		}

	};


}
