#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Render/Texture.h"
#include "Shark/Event/Event.h"
#include "Shark/Event/ApplicationEvent.h"

#include "Panels/Panel.h"

namespace Shark {

	class SceneHirachyPanel : public Panel
	{
	public:
		SceneHirachyPanel(Ref<Scene> scene = nullptr);

		virtual void OnImGuiRender() override;
		virtual void OnEvent(Event& event) override;

		virtual bool IsShown() const override { return m_ShowPanel; }

		void SetContext(Ref<Scene> scene) { m_Context = scene; }
		const Ref<Scene>& GetContext() const { return m_Context; }

		Entity GetSelectedEntity() const { return m_SelectedEntity; }
		void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }

		template<typename Func>
		void SetSelectionChangedCallback(const Func& func) { m_SelectionChangedCallback = func; }

	private:
		void DrawEntityNode(Entity entity);
		void DrawEntityProperties(Entity entity);

		void DestroyEntity(Entity entity);
		void SelectEntity(Entity entity);
	private:
		Ref<Scene> m_Context;
		Entity m_SelectedEntity;

		std::function<void(Entity entity)> m_SelectionChangedCallback = [](auto) {};

		int m_SelectedProjectionIndex = -1;
		static constexpr const char* s_ProjectionItems[] = { "Perspective", "Orthographic" };
		static constexpr const char* s_GeomatryTypes[] = { "Quad", "Circle" };
		static constexpr const char* s_BodyTypes[] = { "Static", "Dynamic", "Kinematic" };

		bool m_ScriptFound = false;

		bool m_ShowPanel = true;
	};

}