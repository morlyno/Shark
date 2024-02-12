#pragma once

#include "Panel.h"
#include "Shark/UI/UI.h"

namespace Shark {

	class EditorPanel : public RefCount
	{
	public:
		EditorPanel(const std::string& panelName)
			: m_PanelName(panelName) {}
		virtual ~EditorPanel() = default;

		virtual void OnUpdate(TimeStep ts) {};
		virtual void OnImGuiRender(bool& shown, bool& destroy) {};
		virtual void OnEvent(Event& event) {};

		virtual void DockWindow(ImGuiID dockspaceID) = 0;
		virtual void SetAsset(const AssetMetaData& metadata) = 0;

		void SetPanelName(const std::string& name) { m_PanelName = name; }
		const std::string& GetPanelName() const { return m_PanelName; }

	protected:
		std::string m_PanelName;
	};

	class AssetEditorManagerPanel : public Panel
	{
	private:
		struct EditorPanelEntry
		{
			Ref<EditorPanel> Editor;
			bool Shown;
		};

	public:
		AssetEditorManagerPanel(const std::string& panelName);
		virtual ~AssetEditorManagerPanel();

		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		ImGuiID GetDockspaceID() const { return m_DockspaceID; }

		template<typename T>
		Ref<T> AddEditor(const AssetMetaData& metadata)
		{
			if (m_EditorPanels.contains(metadata.Handle))
				return m_EditorPanels.at(metadata.Handle).Editor.As<T>();

			Ref<T> assetEditor = Ref<T>::Create(FileSystem::GetStemString(metadata.FilePath), metadata);
			m_EditorPanels[metadata.Handle] = { assetEditor, true };

			assetEditor->DockWindow(m_DockspaceID);

			return assetEditor;
		}

	private:
		void DrawPanels();

	private:
		std::unordered_map<AssetHandle, EditorPanelEntry> m_EditorPanels;
		ImGuiID m_DockspaceID;
	};

}	
