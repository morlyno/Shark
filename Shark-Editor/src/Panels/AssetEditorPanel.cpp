#include "AssetEditorPanel.h"

#include "Shark/Asset/AssetMetadata.h"

#include "Shark/UI/UICore.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	////////////////////////////////////////////////////////////////////////////////////////////////////
	///// Editor Panel /////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	void EditorPanel::DockWindow(ImGuiID dockspaceID)
	{
		m_DockspaceID = dockspaceID;
		m_DockWindow = true;
	}

	void EditorPanel::DockIfNeeded()
	{
		if (m_DockWindow)
		{
			ImGui::SetNextWindowDockID(m_DockspaceID, ImGuiCond_Always);
			m_DockWindow = false;
		}
	}

	void EditorPanel::OnImGuiRender(bool& showWindow)
	{
		if (m_DockWindow)
		{
			ImGui::SetNextWindowDockID(m_DockspaceID, ImGuiCond_Always);
			m_DockWindow = false;
		}

		if (ImGui::Begin(m_PanelName.c_str(), &showWindow, GetWindowFlags()))
		{
			DrawWindow(showWindow);
		}
		ImGui::End();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	///// Asset Editor Manager Panel ///////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	AssetEditorManagerPanel::AssetEditorManagerPanel()
	{
		m_DockspaceID = ImGui::GetIDWithSeed("AssetEditorPanelDockspace", nullptr, (uint32_t)(uint64_t)this);
	}

	AssetEditorManagerPanel::~AssetEditorManagerPanel()
	{
	}

	void AssetEditorManagerPanel::OnUpdate(TimeStep ts)
	{
		for (auto& [id, editor] : m_EditorPanels)
			editor->OnUpdate(ts);
	}

	void AssetEditorManagerPanel::OnImGuiRender(bool& shown)
	{
		SK_PROFILE_FUNCTION();

		if (m_EditorPanels.empty())
			shown = false;

		if (!shown)
			return;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
		const bool opened = ImGui::Begin(m_PanelName, &shown);
		ImGui::PopStyleVar();

		if (opened)
		{
			ImGui::DockSpace(m_DockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_NoWindowMenuButton);
			DrawPanels();
		}

		ImGui::End();
	}

	void AssetEditorManagerPanel::OnEvent(Event& event)
	{
		for (auto& [id, editor] : m_EditorPanels)
			editor->OnEvent(event);
	}

	void AssetEditorManagerPanel::OpenEditor(const AssetMetaData& metadata, Ref<EditorPanel> editorPanel)
	{
		m_EditorPanels[metadata.Handle] = editorPanel;
		editorPanel->DockWindow(m_DockspaceID);
	}

	void AssetEditorManagerPanel::DrawPanels()
	{
		for (auto i = m_EditorPanels.begin(); i != m_EditorPanels.end();)
		{
			auto& editor = i->second;

			bool showWindow = true;
			editor->OnImGuiRender(showWindow);
			if (!showWindow)
			{
				i = m_EditorPanels.erase(i);
				continue;
			}

			++i;
		}
	}

}
