#pragma once

#include <Shark/Core/Base.h>
#include <Shark/Scene/Scene.h>
#include <Shark/Scene/Entity.h>
#include <Shark/Render/Texture.h>
#include <Shark/Event/Event.h>
#include <Shark/Event/ApplicationEvent.h>

namespace Shark {

	class SceneHirachyPanel
	{
	public:
		SceneHirachyPanel() = default;
		SceneHirachyPanel(const Ref<Scene>& context);
		void SetContext(const Ref<Scene>& context);
		const Ref<Scene>& GetContext() const { return m_Context; }

		void ShowPanel(bool show) { m_ShowPanel = show; }
		bool IsShowen() const { return m_ShowPanel; }
		void ShwoProerties(bool show) { m_ShowProperties = show; }
		bool PropertiesShown() const { return m_ShowProperties; }

		Entity GetSelectedEntity() const { return m_SelectedEntity; }

		void ScenePlaying(bool playing) { m_ScenePlaying = playing; }

		void OnImGuiRender();

		void OnEvent(Event& event);

	private:
		void DrawEntityNode(Entity entity);
		void DrawEntityProperties(Entity entity);

		bool OnSelectionChanged(SelectionChangedEvent& event);
	private:
		bool m_ShowPanel = true;
		bool m_ShowProperties = false;

		Ref<Scene> m_Context;
		Entity m_SelectedEntity;

		bool m_ScenePlaying = false;

		int m_SelectedProjectionIndex = -1;
		static constexpr const char* s_ProjectionItems[] = { "Perspective", "Orthographic" };
		static constexpr const char* s_GeomatryTypes[] = { "Quad", "Circle" };
		static constexpr const char* s_BodyTypes[] = { "Static", "Dynamic", "Kinematic" };

		bool m_ScriptFound = false;

		std::string m_FilePathInputBuffer;
	};

}