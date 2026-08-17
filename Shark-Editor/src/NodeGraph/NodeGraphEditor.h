#pragma once

#include "Shark/Core/Base.h"
#include "Shark/UI/UICore.h"

#include "Panels/AssetEditorPanel.h"
#include "NodeGraph/EditorNodes.h"

#include <imgui_node_editor.h>
#include <choc/containers/choc_Value.h>

namespace Shark {
	class Scene;

	namespace NodeGraph {
		struct ProcessNode;
		struct NodeContext;
		class Prototype;
	}

	namespace NodeGraph::Editor {
		class NodeGraphContext;
		class AbstractFactory;
	}
}

namespace Shark::NodeGraph::Editor {

	class NodeGraphEditor : public EditorPanel
	{
	public:
		NodeGraphEditor(const std::string& name, const AssetMetaData& metadata);
		~NodeGraphEditor();

		virtual void OnImGuiRender(bool& isOpen) final;
		//virtual void SetContext(Ref<Scene> context);

		void Draw();
		void DrawCanvas();
		void DrawGraphIO();
		void DrawProperty();

	protected:
		virtual void OnCompileGraph() {}
		virtual void OnPlayGraph() {}
		virtual void OnDrawGraphIO() {}

		void SetGraphContext(Scope<NodeGraphContext> graphContext);
		NodeGraphContext* GetGraphContext();

	private:
		void SelectInput(std::string_view name);
		bool DrawPinValueEdit(Pin* pin);

	private:
		ax::NodeEditor::EditorContext* m_EditorContext;
		Scope<NodeGraphContext> m_Context;

		ImGuiID m_DockspaceID;
		ImGuiWindowClass m_WindowClass;
		std::string m_PropertiesWindowID;

		Ref<Scene> m_Scene;

		struct CurrentState
		{
			ax::NodeEditor::NodeId ContextNodeId = 0;
			ax::NodeEditor::LinkId ContextLinkId = 0;
			ax::NodeEditor::PinId  ContextPinId = 0;
			bool CreateNewNode = false;
			ax::NodeEditor::PinId NewNodeLinkPinID = 0;
			const Pin* NewLinkPin = nullptr;
		};

		struct SelectedInput
		{
			std::string Name;

			std::string RenameBuffer;
		};

		struct EntityPopupContext
		{
			Pin* EntityPin = nullptr;
			bool OpenPopup = false;
			ImVec2 Position = { 0, 0 };
			ImGuiID PopupID = UI::GenerateUniqueID();

			void Set(Pin* pin, ImVec2 position = { 0, 0 })
			{
				EntityPin = pin;
				OpenPopup = pin != nullptr;
				Position = position;
			}
		};

		struct AssetPopupContext
		{
			Pin* AssetPin = nullptr;
			bool OpenPopup = false;
			ImVec2 Position = { 0, 0 };
			ImGuiID PopupID = UI::GenerateUniqueID();

			void Set(Pin* pin, ImVec2 position = { 0, 0 })
			{
				AssetPin = pin;
				OpenPopup = pin != nullptr;
				Position = position;
			}
		};

		CurrentState       m_CurrentState;
		SelectedInput      m_SelectedInput;
		EntityPopupContext m_EntityPinPopup;
		AssetPopupContext  m_AssetPinPopup;

	};

}

namespace Shark {
	using NodeGraph::Editor::NodeGraphEditor;
}
