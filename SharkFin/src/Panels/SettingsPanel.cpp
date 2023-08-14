#include "skfpch.h"
#include "SettingsPanel.h"

#include "Shark/UI/UI.h"

namespace Shark {

	static SettingsPanel* s_Instance = nullptr;

	SettingsPanel::SettingsPanel(const std::string& panelName)
		: Panel(panelName)
	{
		SK_CORE_ASSERT(s_Instance == nullptr);
		s_Instance = this;
	}

	SettingsPanel::~SettingsPanel()
	{
		s_Instance = nullptr;
	}

	void SettingsPanel::OnImGuiRender(bool& shown)
	{
		if (!shown)
			return;

		if (!ImGui::Begin(m_PanelName.c_str(), &shown))
		{
			ImGui::End();
			return;
		}

		for (const auto& func : m_Nodes)
			func();

		ImGui::End();
	}

	Ref<SettingsPanel> SettingsPanel::Get()
	{
		return s_Instance;
	}
}
