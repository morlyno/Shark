#include "skfpch.h"
#include "PanelManager.h"

#include "Panels/SceneHirachyPanel.h"
#include "Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Panels/TextureEditorPanel.h"

#include "Shark/Debug/Profiler.h"

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

	void PanelManager::DrawPanelsMenu(const char* menuName)
	{
		if (ImGui::BeginMenu(menuName))
		{
			for (auto& [id, data] : m_Panels)
				ImGui::MenuItem(data.Instance->GetName(), nullptr, &data.Shown);

			ImGui::EndMenu();
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

	void PanelManager::OnProjectChanged(Ref<ProjectInstance> project)
	{
		for (auto& [id, data] : m_Panels)
			data.Instance->OnProjectChanged(project);
	}

}
