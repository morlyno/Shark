#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Render/Texture.h"

#include "Shark/Event/Event.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Event/ApplicationEvent.h"

#include "Shark/Editor/Panel.h"

namespace Shark {

	class SceneHirachyPanel : public Panel
	{
	public:
		SceneHirachyPanel(Ref<Scene> scene = nullptr);

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;

		void SetContext(Ref<Scene> scene) { m_Context = scene; }
		const Ref<Scene>& GetContext() const { return m_Context; }

		Entity GetSelectedEntity() const { return m_SelectedEntity; }
		void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }

		template<typename Func>
		void SetSelectionChangedCallback(const Func& func) { m_SelectionChangedCallback = func; }

	private:
		bool OnKeyPressedEvent(KeyPressedEvent& event);

		void DrawEntityNode(Entity entity);
		void DrawEntityProperties(Entity entity);
		void DrawAppEntityPopup();

		void DestroyEntity(Entity entity);
		void SelectEntity(Entity entity);
	private:
		Ref<Scene> m_Context;
		Entity m_SelectedEntity;

		bool m_HirachyFocused = false;
		bool m_PropertiesFocused = false;

		std::function<void(Entity entity)> m_SelectionChangedCallback = [](auto) {};

		static constexpr const char* s_ProjectionItems[] = { "None", "Perspective", "Orthographic" };
		static constexpr const char* s_GeomatryTypes[] = { "Quad", "Circle" };
		static constexpr std::string_view s_BodyTypes[] = { "Static", "Dynamic", "Kinematic" };

		bool m_ScriptFound = false;
	};

}