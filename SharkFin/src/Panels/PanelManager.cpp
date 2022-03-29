#include "skfpch.h"
#include "PanelManager.h"

#include "Panels/SceneHirachyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/TextureEditorPanel.h"
#include "Shark/Utility/Utility.h"

namespace Shark {

	void PanelManager::AddPanel(std::string id, Ref<Panel> panel)
	{
		m_Panels[id] = panel;
	}

	void PanelManager::RemovePanel(std::string panelID)
	{
		m_Panels.erase(panelID);
	}

	bool PanelManager::HasPanel(std::string id)
	{
		return Utility::Contains(m_Panels, id);
	}

	Ref<Panel> PanelManager::GetPanel(std::string id)
	{
		return m_Panels[id];
	}



	Ref<Panel> PanelManager::GetEditor(std::string id, uint32_t index)
	{
		return m_EditorPanels[id][index];
	}

	void PanelManager::AddEditor(std::string id, Ref<Panel> panel)
	{
		m_EditorPanels[id].emplace_back(panel);
	}

	stl::vector_view<Ref<Panel>> PanelManager::GetEditors(std::string id)
	{
		auto entry = m_EditorPanels.find(id);
		if (entry != m_EditorPanels.end())
			return entry->second;
		return stl::vector_view<Ref<Panel>>();
	}

	void PanelManager::RemoveEditor(std::string id, uint32_t index)
	{
		auto& v = m_EditorPanels[id]; if (v.size() == 1) (m_EditorPanels.erase(id)); else (v.erase(v.begin() + index));
	}

	void PanelManager::RemoveEditors(std::string id)
	{
		m_EditorPanels.erase(id);
	}

	void PanelManager::CheckEditorsWantDestroy()
	{
		for (auto entry = m_EditorPanels.begin(); entry != m_EditorPanels.end();)
		{
			auto& vec = entry->second;
			for (auto elem = vec.begin(); elem != vec.end();)
			{
				if ((*elem)->WantDestroy())
				{
					elem = entry->second.erase(elem);
					continue;
				}
				elem++;
			}
			if (vec.empty())
			{
				entry = m_EditorPanels.erase(entry);
				continue;
			}
			entry++;
		}
	}



	void PanelManager::OnUpdate(TimeStep ts)
	{
		for (auto [id, panel] : m_Panels)
			panel->OnUpdate(ts);

		CheckEditorsWantDestroy();
		
		for (auto [id, vec] : m_EditorPanels)
			for (auto panel : vec)
				panel->OnUpdate(ts);
	}

	void PanelManager::OnImGuiRender()
	{
		for (auto [id, panel] : m_Panels)
			panel->OnImGuiRender();

		for (auto [id, vec] : m_EditorPanels)
			for (auto panel : vec)
				panel->OnImGuiRender();
	}

	void PanelManager::OnEvent(Event& event)
	{
		for (auto [id, panel] : m_Panels)
			panel->OnEvent(event);

		for (auto [id, vec] : m_EditorPanels)
			for (auto panel : vec)
				panel->OnEvent(event);
	}

}
