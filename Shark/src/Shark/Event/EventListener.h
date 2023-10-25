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
		void OnWindowResizeEvent(uint32_t width, uint32_t height);
		void OnWindowMaximizedEvent(bool maximized);
		void OnWindowMinimizedEvent(bool minimized);
		void OnWindowMoveEvent(int x, int y);
		void OnWindowFocusEvent();
		void OnWindowLostFocusEvent();
		void OnWindowDropEvent(const std::vector<std::filesystem::path>& paths);
		void OnWindowDropEvent(std::vector<std::filesystem::path>&& paths);

		void OnMouseMovedEvent(float x, float y);
		void OnMouseButtonPressedEvent(MouseButton button);
		void OnMouseButtonReleasedEvent(MouseButton button);
		void OnMouseButtonDoubleClickedEvent(MouseButton button);
		void OnMouseScrolledEvent(float xOffset, float yOffset);

		void OnKeyPressedEvent(KeyCode key, bool isRepead, const ModifierKeys& modifierKeys);
		void OnKeyReleasedEvent(KeyCode key, const ModifierKeys& modifierKeys);

		void OnApplicationClosedEvent();
#if 0
		void OnSceneChagedEvent(Ref<Scene> scene);
		void OnScenePlayEvent(Ref<Scene> scene);
		void OnProjectChangedEvent(Ref<ProjectInstance> project);
#endif

	private:
		template<typename TEvent, typename... TArgs>
		void OnEvent(TArgs&&... args)
		{
			TEvent event{ std::forward<TArgs>(args)... };
			m_Callback(event);
		}

	private:
		std::function<void(Event& event)> m_Callback;
	};

}
