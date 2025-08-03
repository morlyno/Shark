#pragma once

#include "Panel.h"
#include <magic_enum_containers.hpp>

namespace Shark {

	enum class PanelCategory
	{
		Edit, View
	};

	struct PanelData
	{
		const char* ID = nullptr;
		const char* Name = nullptr;
		PanelCategory Category;
		Ref<Panel> Panel;
		bool IsOpen = false;
	};

	class PanelManager
	{
	public:
		PanelManager();
		~PanelManager();

		void AddPanel(PanelCategory category, const char* id, const char* name, bool isDefaultOpen, Ref<Panel> panel);
		void RemovePanel(const char* panelID);
		void RemoveAll();

		Ref<Panel> GetPanel(const char* panelID) const;
		std::span<PanelData> GetPanels(PanelCategory category) { return m_PanelsPerCategory[category]; }

		void ShowPanel(const char* panelID);
		void HidePanel(const char* panelID);

		void LoadSettings();
		void SaveSettings();

		void DrawMenus();

	public:
		template<typename TPanel, typename... TArgs>
		Ref<TPanel> AddPanel(PanelCategory category, const char* name, bool isDefaultOpen, TArgs&&... args);

		template<typename TPanel>
		Ref<TPanel> GetPanel() const;

		template<typename TPanel>
		void ShowPanel();
		
		template<typename TPanel>
		void HidePanel();

	public:
		void OnUpdate(TimeStep ts);
		void OnImGuiRender();
		void OnEvent(Event& event);

		void SetContext(Ref<Scene> context);
		void OnScenePlay();
		void OnSceneStop();
		void OnProjectChanged(Ref<ProjectConfig> projectConfig);

	private:
		PanelData* GetPanelData(const char* panelID);
		PanelData* GetPanelData(PanelCategory category, const char* panelID);
		const PanelData* GetPanelData(const char* panelID) const;
		const PanelData* GetPanelData(PanelCategory category, const char* panelID) const;

		template<typename TFunction, typename... TArgs>
		void Call(const TFunction& function, TArgs&&... args)
		{
			for (auto& panels : m_PanelsPerCategory)
			{
				for (auto& panelData : panels)
				{
					std::invoke(function, *panelData.Panel.Raw(), std::forward<TArgs>(args)...);
				}
			}
		}

	private:
		magic_enum::containers::array<PanelCategory, std::vector<PanelData>> m_PanelsPerCategory;
		std::unordered_map<std::string, bool> m_Settings;
	};

	template<typename TPanel, typename... TArgs>
	Ref<TPanel> PanelManager::AddPanel(PanelCategory category, const char* name, bool isOpen, TArgs&&... args)
	{
		Ref<TPanel> panel = Ref<TPanel>::Create(std::forward<TArgs>(args)...);
		AddPanel(category, TPanel::GetStaticID(), name, isOpen, panel);
		return panel;
	}

	template<typename TPanel>
	Ref<TPanel> PanelManager::GetPanel() const
	{
		return GetPanel(TPanel::GetStaticID()).As<TPanel>();
	}

	template<typename TPanel>
	void PanelManager::ShowPanel()
	{
		return ShowPanel(TPanel::GetStaticID());
	}

	template<typename TPanel>
	void PanelManager::HidePanel()
	{
		return HidePanel(TPanel::GetStaticID());
	}

}
