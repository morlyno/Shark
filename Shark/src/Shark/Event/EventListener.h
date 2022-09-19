#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Core/Project.h"
#include "Shark/Event/Event.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Input/KeyCodes.h"
#include "Shark/Input/MouseButtons.h"
#include "Shark/Scene/Scene.h"

namespace Shark {

	class EventListener : public RefCount
	{
	public:
		EventListener(const std::function<void(Event&)>& callback);
		~EventListener();

		void OnWindowCloseEvent();
		void OnWindowResizeEvent(uint32_t width, uint32_t height, WindowResizeEvent::State state);
		void OnWindowMoveEvent(int x, int y);
		void OnWindowFocusEvent();
		void OnWindowLostFocusEvent();

		void OnMouseMovedEvent(const glm::ivec2& mousePos);
		void OnMouseButtonPressedEvent(const glm::ivec2& mousePos, MouseButton button);
		void OnMouseButtonReleasedEvent(const glm::ivec2& mousePos, MouseButton button);
		void OnMouseButtonDoubleClickedEvent(const glm::ivec2& mousePos, MouseButton button);
		void OnMouseScrolledEvent(const glm::ivec2& mousePos, float delta);

		void OnKeyPressedEvent(KeyCode key, bool isRepead);
		void OnKeyReleasedEvent(KeyCode key);

		void OnApplicationClosedEvent();
		void OnSceneChagedEvent(Ref<Scene> scene);
		void OnScenePlayEvent(Ref<Scene> scene);
		void OnProjectChangedEvent(Ref<Project> project);

	private:
		template<typename TEvent, typename... TArgs>
		void OnEvent(TArgs&&... args)
		{
			m_Callback(TEvent(args...));
		}

	private:
		std::function<void(Event& event)> m_Callback;
	};

}
