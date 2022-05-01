#pragma once

#include "Panels/Panel.h"

#include "Shark/stl/container_view.h"

namespace Shark {

	// Editor Panels?
	// could use differnt id method
	// probably no access necessary
	// remove/delete editor panel?
	// - custom callback
	// - flag that indicates destroction
	// 

	class PanelManager
	{
	public:
		template<typename T, typename... Args>
		Ref<T> AddPanel(std::string id, bool show, Args&&... args)
		{
			auto panel = Ref<T>::Create(std::forward<Args>(args)...);
			AddPanel(id, panel, show);
			return panel;
		}
		template<typename T>
		Ref<T> GetPanel(std::string id) const
		{
			return m_Panels.at(id).Instance.As<T>();
		}

		void AddPanel(std::string id, Ref<Panel> panel, bool show);
		void RemovePanel(std::string panelID);
		bool HasPanel(std::string id) const;
		Ref<Panel> GetPanel(std::string id) const;
		bool IsShown(std::string id) const
		{
			return m_Panels.at(id).Shown;
		}

		template<typename T, typename... Args>
		Ref<T> AddEditor(std::string id, Args&&... args)
		{
			auto panel = Ref<T>::Create(std::forward<Args>(args)...);
			AddEditor(id, panel);
			return panel;
		}
		template<typename T, typename... Args>
		Ref<T> GetEditor(std::string id, uint32_t index = 0) const
		{
			return GetEditor(id, index).As<T>();
		}

		void AddEditor(std::string id, Ref<Panel> panel);
		Ref<Panel> GetEditor(std::string id, uint32_t index = 0) const;
		stl::vector_view<Ref<Panel>> GetEditors(std::string id) const;

		void RemoveEditor(std::string id, uint32_t index = 0);
		void RemoveEditors(std::string id);

	private:
		void CheckEditorsWantDestroy();

	public:
		void OnUpdate(TimeStep ts);
		void OnImGuiRender();
		void OnEvent(Event& event);
	private:
		// sorted?
		struct PanelData
		{
			Ref<Panel> Instance;
			bool Shown;
		};
		std::unordered_map<std::string, PanelData> m_Panels;
		std::unordered_map<std::string, std::vector<Ref<Panel>>> m_EditorPanels;
	};

}
