#include "skfpch.h"
#include "AssetEditorPanel.h"

#include "Shark/UI/UI.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	AssetEditorPanel::AssetEditorPanel(const std::string& panelName)
		: Panel(panelName)
	{
		m_DockspaceID = UI::GetIDWithSeed("AssetEditorPanelDockspace", (uint32_t)(uint64_t)this);
	}

	AssetEditorPanel::~AssetEditorPanel()
	{
	}

	void AssetEditorPanel::OnUpdate(TimeStep ts)
	{
		for (auto& [id, entry] : m_EditorPanels)
			entry.Editor->OnUpdate(ts);
	}

	void AssetEditorPanel::OnImGuiRender(bool& shown)
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

	void AssetEditorPanel::OnEvent(Event& event)
	{
		for (auto& [id, entry] : m_EditorPanels)
			entry.Editor->OnEvent(event);
	}

	bool AssetEditorPanel::AnyViewportHovered() const
	{
		for (auto& [id, entry] : m_EditorPanels)
			if (entry.Editor->IsViewportHovered())
				return true;
		return false;
	}

	void AssetEditorPanel::DrawPanels()
	{
		for (auto it = m_EditorPanels.begin(); it != m_EditorPanels.end();)
		{
			auto& entry = it->second;
			entry.Editor->OnImGuiRender(entry.Shown, entry.Destroy);
			if (entry.Destroy)
			{
				it = m_EditorPanels.erase(it);
				continue;
			}

			it++;
		}
	}

}
