#pragma once

#include "Shark/Core/UUID.h"
#include "NodeGraph/Identifier.h"

#include <imgui_node_editor.h>
#include <choc/containers/choc_Value.h>

namespace Shark {

	namespace GraphEditor {

		enum class PinType
		{
			Flow,
			Bool,
			Int,
			Float,
		};

		template<typename T>
		static constexpr PinType GetPinType()
		{
			if constexpr (std::is_same_v<T, bool>)
				return PinType::Bool;
			else if constexpr (std::is_same_v<T, int>)
				return PinType::Int;
			else if constexpr (std::is_same_v<T, float>)
				return PinType::Float;
			else
				static_assert(false, "Invalid type");
		}


		struct Pin
		{
			ax::NodeEditor::PinId   ID;
			ax::NodeEditor::NodeId  NodeID;
			ax::NodeEditor::PinKind Kind = ax::NodeEditor::PinKind::Input;
			std::string             Name;

			Identifier Identifier;
			PinType Type;
			choc::value::Value Value;

			UUID GetNodeID() const { return UUID::Make(NodeID.Get()); }
		};

		struct Node
		{
			ax::NodeEditor::NodeId ID;
			std::string Name;
			std::vector<Pin> Inputs;
			std::vector<Pin> Outputs;
			ImColor Color;
			ImVec2 Size;

			std::string State;
			std::string SavedState;

			UUID GetID() const { return UUID::Make(ID.Get()); }
		};

		struct Link
		{
			ax::NodeEditor::LinkId ID;

			ax::NodeEditor::PinId StartPinID;
			ax::NodeEditor::PinId EndPinID;

			ImColor Color;
		};

	}

}
