#pragma once

#include "Shark/Event/Event.h"
#include "Shark/Input/MouseButtons.h"

namespace Shark {

	class MouseMovedEvent : public EventBase<EventType::MouseMoved, EventCategory::Input | EventCategory::Mouse>
	{
	public:
		MouseMovedEvent(float x, float y)
			: m_MouseX(x), m_MouseY(y) {}

		float GetX() const { return m_MouseX; }
		float GetY() const { return m_MouseY; }
		std::string ToString() const override { return fmt::format("{}, Pos: {}", GetName(), glm::vec2(m_MouseX, m_MouseY)); }

	private:
		float m_MouseX, m_MouseY;
	};

	class MouseButtonPressedEvent : public EventBase<EventType::MouseButtonPressed, EventCategory::Input | EventCategory::Mouse>
	{
	public:
		MouseButtonPressedEvent(MouseButton button)
			: m_Button(button) {}

		MouseButton GetButton() const { return m_Button; }
		std::string ToString() const override { return fmt::format("{}, Button: {}", GetName(), m_Button); }

	private:
		MouseButton m_Button;
	};

	class MouseButtonReleasedEvent : public EventBase<EventType::MouseButtonReleasd, EventCategory::Input | EventCategory::Mouse>
	{
	public:
		MouseButtonReleasedEvent(MouseButton button)
			: m_Button(button) {}

		MouseButton GetButton() const { return m_Button; }
		std::string ToString() const override { return fmt::format("{}, Button: {}", GetName(), m_Button); }

	private:
		MouseButton m_Button;
	};

	class MouseButtonDoubleClickedEvent : public EventBase<EventType::MouseButtonDoubleClicked, EventCategory::Input | EventCategory::Mouse>
	{
	public:
		MouseButtonDoubleClickedEvent(MouseButton button)
			: m_Button(button) {}

		MouseButton GetButton() const { return m_Button; }
		std::string ToString() const override { return fmt::format("{}, Button: {}", GetName(), m_Button); }

	private:
		MouseButton m_Button;
	};

	class MouseScrolledEvent : public EventBase<EventType::MouseScrolled, EventCategory::Input | EventCategory::Mouse>
	{
	public:
		MouseScrolledEvent(float xOffset, float yOffset)
			: m_XOffset(xOffset), m_YOffset(yOffset) {}

		float GetXOffset() const { return m_XOffset; }
		float GetYOffset() const { return m_YOffset; }
		std::string ToString() const override { return fmt::format("{}, Offset: {}", GetName(), glm::vec2(m_XOffset, m_YOffset)); }

	private:
		float m_XOffset, m_YOffset;
	};

}