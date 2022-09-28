#include "skfpch.h"
#include "AssetEditorPanel.h"

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	AssetEditorPanel::AssetEditorPanel(const char* panelName)
		: Panel(panelName)
	{
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

	void AssetEditorPanel::OnEvent(Event& event)
	{
		for (auto& [id, entry] : m_EditorPanels)
			entry.Editor->OnEvent(event);
	}

}
