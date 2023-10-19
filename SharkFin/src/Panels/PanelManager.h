#pragma once

#include "Shark/Editor/Panel.h"

namespace Shark {

	class PanelManager
	{
	public:
		template<typename T, typename... Args>
		Ref<T> AddPanel(const std::string& id, const std::string& panelName, bool show, Args&&... args)
		{
			auto panel = Ref<T>::Create(panelName, std::forward<Args>(args)...);
			m_Panels[id] = { panel, show };
			return panel;
		}

		template<typename T = Panel>
		Ref<T> GetPanel(const std::string& id) const { return m_Panels.at(id).Instance.As<T>(); }

		void RemovePanel(const std::string& panelID) { m_Panels.erase(panelID); }
		bool HasPanel(const std::string& id) const { return m_Panels.contains(id); }

		bool IsShown(const std::string& id) const { return m_Panels.at(id).Shown; }
		void Show(const std::string& id, bool shown) { m_Panels.at(id).Shown = shown; }
		bool ToggleShow(const std::string& id) { PanelEntry& entry = m_Panels.at(id); entry.Shown = !entry.Shown; return entry.Shown; }

		void Clear() { m_Panels.clear(); }

	public:
		void OnUpdate(TimeStep ts);
		void OnImGuiRender();
		void OnEvent(Event& event);

		void DrawPanelsMenu(const std::string& menuName);

		void SetContext(Ref<Scene> context);
		void OnScenePlay();
		void OnSceneStop();
		void OnProjectChanged(Ref<ProjectInstance> project);

	private:
		struct PanelEntry
		{
			Ref<Panel> Instance;
			bool Shown;
		};
		std::unordered_map<std::string, PanelEntry> m_Panels;
	};

}
