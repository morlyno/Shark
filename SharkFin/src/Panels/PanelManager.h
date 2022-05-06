#pragma once

#include "Panels/Panel.h"

#include "Shark/stl/container_view.h"

namespace Shark {

	class PanelManager
	{
	public:
		template<typename T, typename... Args>
		Ref<T> AddPanel(const std::string& id, bool show, Args&&... args)
		{
			auto panel = Ref<T>::Create(std::forward<Args>(args)...);
			AddPanel(id, panel, show);
			return panel;
		}
		template<typename T>
		Ref<T> GetPanel(const std::string& id) const
		{
			return m_Panels.at(id).Instance.As<T>();
		}

		void AddPanel(const std::string& id, Ref<Panel> panel, bool show);
		void RemovePanel(const std::string& panelID);
		bool HasPanel(const std::string& id) const;
		Ref<Panel> GetPanel(const std::string& id) const;
		bool IsShown(const std::string& id) const { return m_Panels.at(id).Shown; }
		void Show(const std::string& id, bool shown) { m_Panels.at(id).Shown = shown; }
		bool ToggleShow(const std::string& id) { PanelEntry& entry = m_Panels.at(id); entry.Shown = !entry.Shown; return entry.Shown; }

	public:
		void OnUpdate(TimeStep ts);
		void OnImGuiRender();
		void OnEvent(Event& event);
	private:
		// sorted?
		struct PanelEntry
		{
			Ref<Panel> Instance;
			bool Shown;
		};
		std::unordered_map<std::string, PanelEntry> m_Panels;
	};

}
