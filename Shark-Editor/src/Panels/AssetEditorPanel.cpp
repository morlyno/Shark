#include "AssetEditorPanel.h"

#include "Shark/UI/UICore.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	AssetEditorManagerPanel::AssetEditorManagerPanel(const std::string& panelName)
		: Panel(panelName)
	{
		m_DockspaceID = ImGui::GetIDWithSeed("AssetEditorPanelDockspace", nullptr, (uint32_t)(uint64_t)this);
	}

	AssetEditorManagerPanel::~AssetEditorManagerPanel()
	{
	}

	void AssetEditorManagerPanel::OnUpdate(TimeStep ts)
	{
		for (auto& [id, entry] : m_EditorPanels)
			entry.Editor->OnUpdate(ts);
	}

	void AssetEditorManagerPanel::OnImGuiRender(bool& shown)
	{
		SK_PROFILE_FUNCTION();

		if (m_EditorPanels.empty())
			shown = false;

		if (!shown)
			return;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
		const bool opened = ImGui::Begin(m_PanelName.c_str(), &shown);
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
		for (auto& [id, entry] : m_EditorPanels)
			entry.Editor->OnEvent(event);
	}

	void AssetEditorManagerPanel::AddEditor(const AssetMetaData& metadata, Ref<EditorPanel> editorPanel)
	{
		m_EditorPanels[metadata.Handle] = { editorPanel, true };
		editorPanel->DockWindow(m_DockspaceID);
	}

	void AssetEditorManagerPanel::DrawPanels()
	{
		for (auto it = m_EditorPanels.begin(); it != m_EditorPanels.end();)
		{
			auto& entry = it->second;
			bool destroy = false;
			entry.Editor->OnImGuiRender(entry.Shown, destroy);
			if (destroy)
			{
				it = m_EditorPanels.erase(it);
				continue;
			}

			it++;
		}
	}

}
