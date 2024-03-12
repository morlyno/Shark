#include "skfpch.h"
#include "PanelManager.h"

#include "Shark/UI/UI.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	void PanelManager::AddPanel(PanelCategory category, const std::string& id, const std::string& panelMame, bool showPanel, Ref<Panel> panel)
	{
		if (m_Panels.contains(id))
		{
			SK_CORE_ERROR_TAG("UI", "Panel with ID: {} already added!", id);
			return;
		}

		auto& entry = m_Panels[id];
		entry.Instance = panel;
		entry.Category = category;
		entry.ShowPanel = showPanel;
		m_PanelsPerCategory[category].push_back(id);
	}

	Ref<Panel> PanelManager::Get(const std::string& id) const
	{
		SK_CORE_VERIFY(m_Panels.contains(id), "Panel with ID: {} dosn't exits", id);
		return m_Panels.at(id).Instance;
	}

	void PanelManager::ShowPanel(const std::string& id, bool showPanel)
	{
		SK_CORE_VERIFY(m_Panels.contains(id), "Panel with ID: {} dosn't exits", id);
		m_Panels.at(id).ShowPanel = showPanel;
	}

	void PanelManager::OnUpdate(TimeStep ts)
	{
		for (auto& [id, data] : m_Panels)
			data.Instance->OnUpdate(ts);
	}

	void PanelManager::OnImGuiRender()
	{
		SK_PROFILE_FUNCTION();

		for (auto& [id, data] : m_Panels)
			data.Instance->OnImGuiRender(data.ShowPanel);
	}

	void PanelManager::OnEvent(Event& event)
	{
		for (auto& [id, data] : m_Panels)
		{
			if (event.Handled)
				break;

			data.Instance->OnEvent(event);
		}
	}

	void PanelManager::DrawPanelsMenu()
	{
		if (ImGui::BeginMenu("View"))
		{
			const auto& panels = m_PanelsPerCategory[PanelCategory::View];
			for (const auto& id : panels)
			{
				PanelEntry& entry = m_Panels.at(id);
				ImGui::MenuItem(entry.Instance->GetName().c_str(), nullptr, &entry.ShowPanel);
			}
			ImGui::End();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			const auto& panels = m_PanelsPerCategory[PanelCategory::Edit];
			for (const auto& id : panels)
			{
				PanelEntry& entry = m_Panels.at(id);
				ImGui::MenuItem(entry.Instance->GetName().c_str(), nullptr, &entry.ShowPanel);
			}
			ImGui::End();
		}
	}

	void PanelManager::SetContext(Ref<Scene> context)
	{
		for (auto& [id, data] : m_Panels)
			data.Instance->SetContext(context);
	}

	void PanelManager::OnScenePlay()
	{
		for (auto& [id, data] : m_Panels)
			data.Instance->OnScenePlay();
	}

	void PanelManager::OnSceneStop()
	{
		for (auto& [id, data] : m_Panels)
			data.Instance->OnSceneStop();
	}

	void PanelManager::OnProjectChanged(Ref<Project> project)
	{
		for (auto& [id, data] : m_Panels)
			data.Instance->OnProjectChanged(project);
	}

}
