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
	};

	struct Link
	{
		ax::NodeEditor::LinkId ID;

		ax::NodeEditor::PinId StartPinID;
		ax::NodeEditor::PinId EndPinID;

		ImColor Color;
	};

}
