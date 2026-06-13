#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Hash.h"

#include "NodeGraph/Identifier.h"
#include "NodeGraph/Nodes.h"
#include "Panel.h"

#include <imgui_node_editor.h>

#include <choc/containers/choc_Value.h>
#include <choc/text/choc_StringUtilities.h>

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

	class NodeGraph
	{
	public:
		std::vector<Node*> Nodes;

		std::vector<choc::value::Value> LocalVariables;
		std::vector<choc::value::ValueView> OutputVariables;
		std::vector<std::string> DebugOutputNames;
	};

	class NodeGraphEditor : public Panel
	{
	public:
		virtual const char* GetPanelID() const override { return GetStaticID(); }
		static const char* GetStaticID() { return "NodeGraphEditor"; }

	public:
		virtual bool OnShowPanel() override;
		virtual bool OnHidePanel() override;

		virtual void OnImGuiRender(bool& isOpen) override;
		void Draw();
		void DrawCanvas();
		void DrawVariables();

		Scope<NodeGraph> CompileGraph();

	private:
		struct Input
		{
			std::string Name;
			choc::value::Value Value;
			GraphEditor::PinType Type;
		};

		uint64_t GetNextID() { return UUID::Generate().Value(); }

		GraphEditor::Node* FindNode(ax::NodeEditor::NodeId id);
		GraphEditor::Link* FindLink(ax::NodeEditor::LinkId id);
		GraphEditor::Link* FindLink(ax::NodeEditor::PinId id);
		GraphEditor::Pin* FindPin(ax::NodeEditor::PinId id);

		void RemoveLinks(ax::NodeEditor::PinId id);

		bool IsPinLinked(ax::NodeEditor::PinId id) const;
		bool CanCreateLink(GraphEditor::Pin* pinA, GraphEditor::Pin* pinB);
		bool WouldCreateLoop(GraphEditor::Pin* startPin, GraphEditor::Pin* endPin);

		template<typename T>
		GraphEditor::Node* SpawnNode()
		{
			using N = Shark::NodeType<T>;

			auto& node = m_Nodes.emplace_back();
			node.ID = GetNextID();
			node.Name = choc::text::replace(N::Inputs::Class, "<", " (", ">", ")");

#if 1
			for (auto member : N::Inputs::Members)
			{
				node.Inputs.push_back({
					.ID = GetNextID(),
					.NodeID = node.ID,
					.Kind = ax::NodeEditor::PinKind::Input,
					.Name = std::string(member),
					.Identifier = member,
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
#endif

			N::Inputs::ForEachTypeIndexed([&]<typename TInput>(size_t index)
			{
				node.Inputs[index].Type = GraphEditor::GetPinType<std::remove_pointer_t<TInput>>();
				//node.Inputs[index].Value = choc::value::Value(choc::value::Type::createPrimitive<std::remove_pointer_t<TInput>>());
			});

			N::Outputs::ForEachTypeIndexed([&]<typename TOutput>(size_t index)
			{
				node.Outputs[index].Type = GraphEditor::GetPinType<std::remove_pointer_t<TOutput>>();
				//node.Inputs[index].Value = choc::value::Type::createPrimitive<T>();
			});

			if (!m_NodeAllocators.contains(node.Name))
				m_NodeAllocators[node.Name] = [](UUID id) -> Node* { return new T(id); };

			return &node;
		}
		GraphEditor::Node* SpawnInputNode(std::string_view inputName);

		bool DrawPinValueEdit(GraphEditor::Pin* pin);
		void ChangeInputType(Input& input, GraphEditor::PinType type);
		void RenameInput(Input& input, const std::string& newName);

	private:
		ax::NodeEditor::EditorContext* m_Context;

		std::vector<GraphEditor::Node> m_Nodes;
		std::vector<GraphEditor::Link> m_Links;

		std::vector<Input> m_InputVariables;

		ax::NodeEditor::NodeId contextNodeId = 0;
		ax::NodeEditor::LinkId contextLinkId = 0;
		ax::NodeEditor::PinId  contextPinId = 0;
		bool createNewNode = false;
		GraphEditor::Pin* newNodeLinkPin = nullptr;
		GraphEditor::Pin* newLinkPin = nullptr;

		Scope<NodeGraph> m_NodeGraph;
		std::unordered_map<std::string, Node* (*)(UUID)> m_NodeAllocators;

		ImGuiID m_DockspaceID;
		ImGuiWindowClass m_WindowClass;
		size_t m_SelectedInput = ~0;

		std::string m_RenameBuffer;

		std::string m_PropertiesWindowID;
	};

}
