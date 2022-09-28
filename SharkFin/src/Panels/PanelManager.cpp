#include "skfpch.h"
#include "PanelManager.h"

#include "Panels/SceneHirachyPanel.h"
#include "Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Panels/TextureEditorPanel.h"

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	void PanelManager::OnUpdate(TimeStep ts)
	{
		for (auto& [id, data] : m_Panels)
			data.Instance->OnUpdate(ts);
	}

	void PanelManager::OnImGuiRender()
	{
		for (auto& [id, data] : m_Panels)
			data.Instance->OnImGuiRender(data.Shown);
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
		if (ImGui::BeginMenu("Panels"))
		{
			for (auto& [id, data] : m_Panels)
				ImGui::MenuItem(data.Instance->PanelName, nullptr, &data.Shown);

			ImGui::EndMenu();
		}
	}

}
