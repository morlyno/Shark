#pragma once

#include "Shark/Core/UUID.h"
#include "NodeGraph/Identifier.h"

#include <imgui_node_editor.h>
#include <choc/containers/choc_Value.h>

namespace Shark::NodeGraph::Editor {

	struct Pin
	{
		ax::NodeEditor::PinId   ID;
		ax::NodeEditor::NodeId  NodeID;
		ax::NodeEditor::PinKind Kind = ax::NodeEditor::PinKind::Input;
		std::string             Name;

		Identifier Identifier;
		choc::value::Value Value;

		int PinType = -1;
		ImColor Color = IM_COL32_WHITE;

		UUID GetNodeID() const { return UUID::Make(NodeID.Get()); }

		template<typename TDesc>
		void SetDesc()
		{
			PinType = TDesc::PinType;
			Color = TDesc::Color;
		}

		Pin() = default;
		Pin(std::string_view name, int pinType, ImColor color = IM_COL32_WHITE, ax::NodeEditor::PinKind kind = ax::NodeEditor::PinKind::Input)
			: ID(UUID::Generate().Value()), Name(name), Identifier(name), PinType(pinType), Color(color), Kind(kind)
		{}
	};

	struct NodeSettings
	{
		bool IsSimple = false;
		bool CanEditPins = true;
		std::unordered_map<Identifier, bool> EditPinOverrides;

		bool DrawPinEdit(Identifier id) const { return EditPinOverrides.contains(id) ? EditPinOverrides.at(id) : CanEditPins; }

		NodeSettings& Simple(bool simple) { IsSimple = simple; return *this; }
		NodeSettings& EditPins(bool edit) { CanEditPins = edit; return *this; }
		NodeSettings& AddOverride(Identifier id, bool editable) { EditPinOverrides[id] = editable; return *this; }
	};

	struct Node
	{
		ax::NodeEditor::NodeId ID;
		std::string Name, Category;
		std::vector<Pin> Inputs;
		std::vector<Pin> Outputs;
		ImColor Color;
		ImVec2 Size;
		NodeSettings Settings;

		std::string State;
		std::string SavedState;

		int EvaluationIndex = -1;

		UUID GetID() const { return UUID::Make(ID.Get()); }

		void Initialize()
		{
			if (!ID)
				ID = UUID::Generate().Value();

			for (auto& pin : Inputs)
			{
				pin.NodeID = ID;
				pin.Kind = ax::NodeEditor::PinKind::Input;
			}

			for (auto& pin : Outputs)
			{
				pin.NodeID = ID;
				pin.Kind = ax::NodeEditor::PinKind::Output;
			}
		}

		Node() = default;
		Node(std::string_view name, std::string_view category)
			: ID(UUID::Generate().Value()), Name(name), Category(category)
		{}
	};

	struct Link
	{
		ax::NodeEditor::LinkId ID;

		ax::NodeEditor::PinId StartPinID;
		ax::NodeEditor::PinId EndPinID;

		ImColor Color;
	};

}
