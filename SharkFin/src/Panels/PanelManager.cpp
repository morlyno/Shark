#include "skfpch.h"
#include "PanelManager.h"

#include "Panels/SceneHirachyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/TextureEditorPanel.h"

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	void PanelManager::AddPanel(const std::string& id, Ref<Panel> panel, bool show)
	{
		SK_PROFILE_FUNCTION();

		m_Panels[id] = { panel, show };
	}

	void PanelManager::RemovePanel(const std::string& panelID)
	{
		SK_PROFILE_FUNCTION();

		m_Panels.erase(panelID);
	}

	bool PanelManager::HasPanel(const std::string& id) const
	{
		SK_PROFILE_FUNCTION();

		return m_Panels.find(id) != m_Panels.end();
	}

	Ref<Panel> PanelManager::GetPanel(const std::string& id) const
	{
		SK_PROFILE_FUNCTION();

		return m_Panels.at(id).Instance;
	}


	void PanelManager::OnUpdate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		for (auto& [id, data] : m_Panels)
			data.Instance->OnUpdate(ts);
	}

	void PanelManager::OnImGuiRender()
	{
		SK_PROFILE_FUNCTION();

		for (auto& [id, data] : m_Panels)
			data.Instance->OnImGuiRender(data.Shown);
	}

	void PanelManager::OnEvent(Event& event)
	{
		SK_PROFILE_FUNCTION();

		for (auto& [id, data] : m_Panels)
		{
			if (event.Handled)
				break;

			data.Instance->OnEvent(event);
		}
	}

}
