#pragma once

#include "Shark/Core/Base.h"
#include "Shark/UI/UICore.h"

#include "Panel.h"
#include "NodeGraph/EditorNodes.h"

#include <imgui_node_editor.h>
#include <choc/containers/choc_Value.h>

namespace Shark {
	class Scene;

	namespace NodeGraph {
		struct ProcessNode;
	}

	namespace NodeGraph::Editor {
		class AbstractFactory;
	}
}

namespace Shark::NodeGraph {

	class Graph
	{
	public:
		std::vector<ProcessNode*> Nodes;

		std::vector<choc::value::Value> LocalVariables;
		std::vector<choc::value::ValueView> OutputVariables;
		std::vector<std::string> DebugOutputNames;
	};

}

namespace Shark::NodeGraph::Editor {

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

		Scope<Graph> CompileGraph();

		virtual void SetContext(Ref<Scene> context);
	private:
		struct Input
		{
			std::string Name;
			choc::value::Value Value;
		};

		uint64_t GetNextID() { return UUID::Generate().Value(); }

		Editor::Node* FindNode(ax::NodeEditor::NodeId id);
		Editor::Link* FindLink(ax::NodeEditor::LinkId id);
		Editor::Link* FindLink(ax::NodeEditor::PinId id);
		Editor::Pin* FindPin(ax::NodeEditor::PinId id);

		void RemoveLinks(ax::NodeEditor::PinId id);

		bool IsPinLinked(ax::NodeEditor::PinId id) const;
		bool CanCreateLink(Pin* pinA, Pin* pinB);
		bool WouldCreateLoop(Pin* startPin, Pin* endPin);

		Node* SpawnInputNode(std::string_view inputName);

		bool DrawPinValueEdit(Pin* pin);
		void ChangeInputType(Input& input, const choc::value::Type& newType);
		void RenameInput(Input& input, const std::string& newName);

	private:
		ax::NodeEditor::EditorContext* m_Context;

		std::vector<Node> m_Nodes;
		std::vector<Link> m_Links;

		Scope<AbstractFactory> m_Factory;
		std::vector<Input> m_InputVariables;

		std::vector<std::string> m_InputTypeNames;

		ax::NodeEditor::NodeId contextNodeId = 0;
		ax::NodeEditor::LinkId contextLinkId = 0;
		ax::NodeEditor::PinId  contextPinId = 0;
		bool createNewNode = false;
		Pin* newNodeLinkPin = nullptr;
		Pin* newLinkPin = nullptr;

		ImGuiID m_DockspaceID;
		ImGuiWindowClass m_WindowClass;
		size_t m_SelectedInput = ~0;

		std::string m_RenameBuffer;
		std::string m_PropertiesWindowID;

		Ref<Scene> m_Scene;
		Scope<Graph> m_NodeGraph;

		struct EntityPopupContext
		{
			Pin* EntityPin = nullptr;
			bool Open = false;
			ImVec2 Position = { 0, 0 };
			ImGuiID PopupID = UI::GenerateUniqueID();

			void Set(Pin* pin, ImVec2 position = { 0, 0 })
			{
				EntityPin = pin;
				Open = pin != nullptr;
				Position = position;
			}
		};


		EntityPopupContext m_EntityPinPopup;

	};

}

namespace Shark {
	using NodeGraph::Editor::NodeGraphEditor;
}
