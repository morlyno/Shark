#include "skpch.h"
#include "EventListener.h"

#include "Shark/Event/KeyEvent.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/Event/WindowEvent.h"
#include "Shark/Event/ApplicationEvent.h"


namespace Shark {

	EventListener::EventListener(const std::function<void(Event&)>& callback)
		: m_Callback(callback)
	{
	}

	EventListener::~EventListener()
	{
	}

	void EventListener::OnWindowCloseEvent()
	{
		OnEvent<WindowCloseEvent>();
	}

	void EventListener::OnWindowResizeEvent(uint32_t width, uint32_t height)
	{
		OnEvent<WindowResizeEvent>(width, height);
	}

	void EventListener::OnWindowMaximizedEvent(bool maximized)
	{
		OnEvent<WindowMaximizedEvent>(maximized);
	}

	void EventListener::OnWindowMinimizedEvent(bool minimized)
	{
		OnEvent<WindowMinimizedEvent>(minimized);
	}

	void EventListener::OnWindowMoveEvent(int x, int y)
	{
		OnEvent<WindowMoveEvent>(x, y);
	}

	void EventListener::OnWindowFocusEvent()
	{
		OnEvent<WindowFocusEvent>();
	}

	void EventListener::OnWindowLostFocusEvent()
	{
		OnEvent<WindowLostFocusEvent>();
	}

	void EventListener::OnWindowDropEvent(const std::vector<std::filesystem::path>& paths)
	{
		OnEvent<WindowDropEvent>(paths);
	}

	void EventListener::OnWindowDropEvent(std::vector<std::filesystem::path>&& paths)
	{
		OnEvent<WindowDropEvent>(std::move(paths));
	}

	void EventListener::OnMouseMovedEvent(float x, float y)
	{
		OnEvent<MouseMovedEvent>(x, y);
	}

	void EventListener::OnMouseButtonPressedEvent(MouseButton button)
	{
		OnEvent<MouseButtonPressedEvent>(button);
	}

	void EventListener::OnMouseButtonReleasedEvent(MouseButton button)
	{
		OnEvent<MouseButtonReleasedEvent>(button);
	}

	void EventListener::OnMouseButtonDoubleClickedEvent(MouseButton button)
	{
		OnEvent<MouseButtonDoubleClickedEvent>(button);
	}

	void EventListener::OnMouseScrolledEvent(float xOffset, float yOffset)
	{
		OnEvent<MouseScrolledEvent>(xOffset, yOffset);
	}

	void EventListener::OnKeyPressedEvent(KeyCode key, bool isRepead, const ModifierKeys& modifierKeys)
	{
		OnEvent<KeyPressedEvent>(key, isRepead, modifierKeys);
	}

	void EventListener::OnKeyReleasedEvent(KeyCode key, const ModifierKeys& modifierKeys)
	{
		OnEvent<KeyReleasedEvent>(key, modifierKeys);
	}

	void EventListener::OnApplicationClosedEvent()
	{
		OnEvent<ApplicationClosedEvent>();
	}

#if 0
	void EventListener::OnSceneChagedEvent(Ref<Scene> scene)
	{
		OnEvent<SceneChangedEvent>(scene);
	}

	void EventListener::OnScenePlayEvent(Ref<Scene> scene)
	{
		OnEvent<ScenePlayEvent>(scene);
	}

	void EventListener::OnProjectChangedEvent(Ref<ProjectInstance> project)
	{
		OnEvent<ProjectChangedEvent>(project);
	}
#endif

}
