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

	void EventListener::OnWindowResizeEvent(uint32_t width, uint32_t height, WindowResizeEvent::State state)
	{
		OnEvent<WindowResizeEvent>(width, height, state);
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

	void EventListener::OnMouseMovedEvent(const glm::ivec2& mousePos)
	{
		OnEvent<MouseMovedEvent>(mousePos);
	}

	void EventListener::OnMouseButtonPressedEvent(const glm::ivec2& mousePos, MouseButton button)
	{
		OnEvent<MouseButtonPressedEvent>(mousePos, button);
	}

	void EventListener::OnMouseButtonReleasedEvent(const glm::ivec2& mousePos, MouseButton button)
	{
		OnEvent<MouseButtonReleasedEvent>(mousePos, button);
	}

	void EventListener::OnMouseButtonDoubleClickedEvent(const glm::ivec2& mousePos, MouseButton button)
	{
		OnEvent<MouseButtonDoubleClickedEvent>(mousePos, button);
	}

	void EventListener::OnMouseScrolledEvent(const glm::ivec2& mousePos, float delta)
	{
		OnEvent<MouseScrolledEvent>(mousePos, delta);
	}

	void EventListener::OnKeyPressedEvent(KeyCode key, bool isRepead)
	{
		OnEvent<KeyPressedEvent>(key, isRepead);
	}

	void EventListener::OnKeyReleasedEvent(KeyCode key)
	{
		OnEvent<KeyReleasedEvent>(key);
	}

	void EventListener::OnApplicationClosedEvent()
	{
		OnEvent<ApplicationClosedEvent>();
	}

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

}
