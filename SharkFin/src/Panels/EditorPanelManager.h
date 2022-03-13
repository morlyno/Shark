#pragma once

#include "Panels/Panel.h"

namespace Shark {

	enum : uint32_t
	{
		SCENE_HIRACHY_ID,
		CONTENT_BROWSER_ID,

		PANEL_COUNT
	};

	class EditorPanelManager
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnUpdate(TimeStep ts);
		static void OnImGuiRender();
		static void OnEvent(Event& event);

		static bool AnyViewportHovered();

		static Ref<Panel> GetPanel(uint32_t panelID);

		static void AddPanel(Ref<Panel> panel);

		template<typename T>
		static Ref<T> GetPanel(uint32_t panelID)
		{
			return GetPanel(panelID).As<T>();
		}

		template<typename T, typename... Args>
		static Ref<T> CreatePanel(Args&&... args)
		{
			Ref<T> panel = Ref<T>::Create(std::forward<Args>(args)...);
			AddPanel(panel);
			return panel;
		}

	};

}
