#pragma once

#include "Shark/Asset/AssetTypes.h"
#include "Shark/Asset/AssetMetadata.h"
#include "Shark/File/FileSystem.h"

#include "Panel.h"

#include <imgui.h>

namespace Shark {

	////////////////////////////////////////////////////////////////////////////////////////////////////
	///// Editor Panel /////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	class EditorPanel : public RefCount
	{
	public:
		EditorPanel(const std::string& panelName)
			: m_PanelName(panelName) {}
		virtual ~EditorPanel() = default;

		virtual void DrawWindow(bool& showWindow) {}
		virtual void OnImGuiRender(bool& showWindow);
		virtual void OnUpdate(TimeStep ts) {};
		virtual void OnEvent(Event& event) {};

		virtual void DockWindow(ImGuiID dockspaceID);
		virtual void DockIfNeeded();
		virtual ImGuiWindowFlags GetWindowFlags() const { return ImGuiWindowFlags_None; }

	protected:
		std::string m_PanelName;

	private:
		bool m_DockWindow = false;
		ImGuiID m_DockspaceID = 0;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////
	///// Asset Editor Manager Panel ///////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	class AssetEditorManagerPanel : public Panel
	{
	public:
		AssetEditorManagerPanel();
		virtual ~AssetEditorManagerPanel();

		virtual void OnUpdate(TimeStep ts) override;
		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		static const char* GetStaticID() { return "AssetEditorManagetPanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }

		ImGuiID GetDockspaceID() const { return m_DockspaceID; }

		void OpenEditor(const AssetMetaData& metadata, Ref<EditorPanel> editorPanel);

		template<typename T>
		Ref<T> OpenEditor(const AssetMetaData& metadata)
		{
			Ref<T> assetEditor = Ref<T>::Create(FileSystem::GetStemString(metadata.FilePath), metadata);
			OpenEditor(metadata, assetEditor);
			return assetEditor;
		}

	private:
		void DrawPanels();

	private:
		std::unordered_map<AssetHandle, Ref<EditorPanel>> m_EditorPanels;
		ImGuiID m_DockspaceID;
	};

}	
