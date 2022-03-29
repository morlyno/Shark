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
		Ref<T> AddPanel(std::string id, Args&&... args)
		{
			auto panel = Ref<T>::Create(std::forward<Args>(args)...);
			AddPanel(id, panel);
			return panel;
		}
		template<typename T>
		Ref<T> GetPanel(std::string id)
		{
			return m_Panels[id].As<T>();
		}

		void AddPanel(std::string id, Ref<Panel> panel);
		void RemovePanel(std::string panelID);
		bool HasPanel(std::string id);
		Ref<Panel> GetPanel(std::string id);


		template<typename T, typename... Args>
		Ref<T> AddEditor(std::string id, Args&&... args)
		{
			auto panel = Ref<T>::Create(std::forward<Args>(args)...);
			AddEditor(id, panel);
			return panel;
		}
		template<typename T, typename... Args>
		Ref<T> GetEditor(std::string id, uint32_t index = 0)
		{
			return GetEditor(id, index).As<T>();
		}

		void AddEditor(std::string id, Ref<Panel> panel);
		Ref<Panel> GetEditor(std::string id, uint32_t index = 0);
		stl::vector_view<Ref<Panel>> GetEditors(std::string id);

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
		std::unordered_map<std::string, Ref<Panel>> m_Panels;
		std::unordered_map<std::string, std::vector<Ref<Panel>>> m_EditorPanels;
	};

}
