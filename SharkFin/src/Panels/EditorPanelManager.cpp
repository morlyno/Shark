#include "skfpch.h"
#include "EditorPanelManager.h"

#include "Panels/SceneHirachyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/TextureEditorPanel.h"

namespace Shark {

	static std::vector<Ref<Panel>> s_Panels;

	void EditorPanelManager::Init()
	{
		SK_CORE_ASSERT(s_Panels.empty());
		s_Panels.reserve(5);
		s_Panels.emplace(s_Panels.begin() + SCENE_HIRACHY_ID, Ref<SceneHirachyPanel>::Create());
		s_Panels.emplace(s_Panels.begin() + CONTENT_BROWSER_ID, Ref<ContentBrowserPanel>::Create());
	}

	void EditorPanelManager::Shutdown()
	{
		s_Panels.clear();
	}

	void EditorPanelManager::OnUpdate(TimeStep ts)
	{
		for (auto panel = s_Panels.begin(); panel != s_Panels.end();)
		{
			if ((*panel)->WantDestroy())
			{
				panel = s_Panels.erase(panel);
				continue;
			}

			panel++;
		}


		for (auto panel : s_Panels)
			panel->OnUpdate(ts);
	}

	void EditorPanelManager::OnImGuiRender()
	{
		for (auto panel : s_Panels)
			panel->OnImGuiRender();
	}

	void EditorPanelManager::OnEvent(Event& event)
	{
		for (auto panel : s_Panels)
			panel->OnEvent(event);
	}

	bool EditorPanelManager::AnyViewportHovered()
	{
		for (auto panel : s_Panels)
			if (panel->ViewportHovered())
				return true;
		return false;
	}

	Ref<Panel> EditorPanelManager::GetPanel(uint32_t panelID)
	{
		if (panelID > s_Panels.size())
			return nullptr;

		return s_Panels[panelID];
	}

	void EditorPanelManager::AddPanel(Ref<Panel> panel)
	{
		SK_CORE_ASSERT(s_Panels.size() >= PANEL_COUNT);
		s_Panels.emplace_back(panel);
	}

}
