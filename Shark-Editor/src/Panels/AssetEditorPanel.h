#pragma once

#include "Panel.h"
#include "Shark/File/FileSystem.h"

#include <imgui.h>

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
		virtual AssetHandle GetAsset() const = 0;

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
		AssetEditorManagerPanel();
		virtual ~AssetEditorManagerPanel();

		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		static const char* GetStaticID() { return "AssetEditorManagetPanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }

		ImGuiID GetDockspaceID() const { return m_DockspaceID; }

		void AddEditor(const AssetMetaData& metadata, Ref<EditorPanel> editorPanel);

		template<typename T>
		Ref<T> AddEditor(const AssetMetaData& metadata)
		{
			Ref<T> assetEditor = Ref<T>::Create(FileSystem::GetStemString(metadata.FilePath), metadata);
			AddEditor(metadata, assetEditor);
			return assetEditor;
		}

	private:
		void DrawPanels();

	private:
		std::unordered_map<AssetHandle, EditorPanelEntry> m_EditorPanels;
		ImGuiID m_DockspaceID;
	};

}	
