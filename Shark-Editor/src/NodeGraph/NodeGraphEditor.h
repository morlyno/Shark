#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Hash.h"

#include "NodeGraph/Identifier.h"
#include "NodeGraph/Nodes.h"
#include "Panel.h"

#include <imgui_node_editor.h>

namespace Shark {

	namespace GraphEditor {

		enum class PinType
		{
			Flow,
			Bool,
			Int,
			Float,
			String,
			Object,
			Function,
			Delegate,
		};

		struct Pin
		{
			ax::NodeEditor::PinId   ID;
			ax::NodeEditor::NodeId  NodeID;
			ax::NodeEditor::PinKind Kind = ax::NodeEditor::PinKind::Input;
			std::string             Name;

			Identifier Identifier;
			PinType Type = PinType::Float;
			float Value = 0.0f;

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

	class NodeGraph
	{
	public:
		std::vector<Node*> Nodes;

		std::vector<float> LocalVariables;
		std::vector<float*> OutputVariables;
		std::vector<std::string> DebugOutputNames;
	};

	class NodeGraphEditor : public Panel
	{
	public:
		virtual const char* GetPanelID() const override { return GetStaticID(); }
		static const char* GetStaticID() { return "NodeGraphEditor"; }

	public:
		NodeGraphEditor();
		~NodeGraphEditor();

		virtual bool OnShowPanel() override;
		virtual bool OnHidePanel() override;

		virtual void OnImGuiRender(bool& isOpen) override;

		Scope<NodeGraph> CompileGraph();

	private:
		uint64_t GetNextID() { return UUID::Generate().Value(); }

		GraphEditor::Node* FindNode(ax::NodeEditor::NodeId id);
		GraphEditor::Link* FindLink(ax::NodeEditor::LinkId id);
		GraphEditor::Link* FindLink(ax::NodeEditor::PinId id);
		GraphEditor::Pin* FindPin(ax::NodeEditor::PinId id);

		bool IsPinLinked(ax::NodeEditor::PinId id) const;
		bool CanCreateLink(GraphEditor::Pin* pinA, GraphEditor::Pin* pinB);
		bool WouldCreateLoop(GraphEditor::Pin* startPin, GraphEditor::Pin* endPin);

		template<typename T>
		GraphEditor::Node* CreateNode()
		{
			using N = Shark::NodeType<T>;

			auto& node = m_Nodes.emplace_back();
			node.ID = GetNextID();
			node.Name = N::Inputs::Class;

			for (auto member : N::Inputs::Members)
			{
				node.Inputs.push_back({
					.ID = GetNextID(),
					.NodeID = node.ID,
					.Kind = ax::NodeEditor::PinKind::Input,
					.Name = std::string(member),
					.Identifier = member
				});
			}

			for (auto member : N::Outputs::Members)
			{
				node.Outputs.push_back({
					.ID = GetNextID(),
					.NodeID = node.ID,
					.Kind = ax::NodeEditor::PinKind::Output,
					.Name = std::string(member),
					.Identifier = member
				});
			}

			if (!m_NodeAllocators.contains(node.Name))
				m_NodeAllocators[node.Name] = [](UUID id) -> Node* { return new T(id); };

			return &node;
		}

	private:
		ax::NodeEditor::EditorContext* m_Context;

		std::vector<GraphEditor::Node> m_Nodes;
		std::vector<GraphEditor::Link> m_Links;

		ax::NodeEditor::NodeId contextNodeId = 0;
		ax::NodeEditor::LinkId contextLinkId = 0;
		ax::NodeEditor::PinId  contextPinId = 0;
		bool createNewNode = false;
		GraphEditor::Pin* newNodeLinkPin = nullptr;
		GraphEditor::Pin* newLinkPin = nullptr;

		Scope<NodeGraph> m_NodeGraph;

		std::unordered_map<std::string, Node* (*)(UUID)> m_NodeAllocators;
	};

}
