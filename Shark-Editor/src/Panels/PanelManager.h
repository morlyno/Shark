#pragma once

#include "Panel.h"

namespace Shark {

	enum class PanelCategory
	{
		None = 0, Edit, View
	};

	class PanelManager
	{
	public:
		PanelManager();

		void AddPanel(PanelCategory category, const std::string& id, const std::string& panelMame, bool showPanel, Ref<Panel> panel);
		Ref<Panel> Get(const std::string& id) const;

		void ShowPanel(const std::string& id, bool showPanel);
		void Clear() { m_Panels.clear(); }

		template<typename TPanel, typename... TArgs>
		Ref<TPanel> AddPanel(PanelCategory category, const std::string& id, const std::string& panelName, bool showPanel, TArgs&&... args)
		{
			Ref<TPanel> panel = Ref<TPanel>::Create(panelName, std::forward<TArgs>(args)...);
			AddPanel(category, id, panelName, showPanel, panel);
			return panel;
		}

		template<typename TPanel>
		Ref<TPanel> Get(const std::string& id) const
		{
			return Get(id).As<TPanel>();
		}

	public:
		void OnUpdate(TimeStep ts);
		void OnImGuiRender();
		void OnEvent(Event& event);

		void DrawPanelsMenu();

		void SetContext(Ref<Scene> context);
		void OnScenePlay();
		void OnSceneStop();
		void OnProjectChanged(Ref<ProjectConfig> projectConfig);

	public:
		void LoadSettings();
		void SaveSettings();

	private:
		struct PanelEntry
		{
			Ref<Panel> Instance;
			PanelCategory Category = PanelCategory::None;
			bool Show = false;
		};
		std::map<std::string, PanelEntry> m_Panels;
		std::map<PanelCategory, std::vector<std::string>> m_PanelsPerCategory;
	};

}
