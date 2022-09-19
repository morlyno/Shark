#pragma once
#include "Shark/Core/Base.h"
#include "Event.h"
#include "Shark/Input/MouseButtons.h"

namespace Shark {

	class MouseEvent : public Event
	{
	public:
		virtual const glm::ivec2& GetMousePos() const = 0;
		virtual int GetX() const = 0;
		virtual int GetY() const = 0;
		virtual MouseButton GetButton() const = 0;
	};

	class MouseMovedEvent : public EventBase<MouseEvent, EventType::MouseMoved, EventCategory::Input | EventCategory::Mouse>
	{
	public:
		MouseMovedEvent(const glm::ivec2& mousepos)
			: m_MousePos(mousepos)
		{}

		virtual const glm::ivec2& GetMousePos() const override { return m_MousePos; }
		virtual int GetX() const override { return m_MousePos.x; }
		virtual int GetY() const override { return m_MousePos.y; }
		virtual MouseButton GetButton() const override { return MouseButton::None; }

		std::string ToString() const override { return fmt::format("{}, Pos: {}", GetName(), m_MousePos); }

	private:
		glm::ivec2 m_MousePos;
	};

	class MouseButtonPressedEvent : public EventBase<MouseEvent, EventType::MouseButtonPressed, EventCategory::Input | EventCategory::Mouse>
	{
	public:
		MouseButtonPressedEvent(const glm::ivec2& mousePos, MouseButton button)
			: m_MousePos(mousePos), m_Button(button)
		{}

		virtual const glm::ivec2& GetMousePos() const override { return m_MousePos; }
		virtual int GetX() const override { return m_MousePos.x; }
		virtual int GetY() const override { return m_MousePos.y; }
		virtual MouseButton GetButton() const override { return m_Button; }

		std::string ToString() const override { return fmt::format("{}, Pos: {}, Button: {}", GetName(), m_MousePos, ::Shark::ToString(m_Button)); }

	private:
		glm::ivec2 m_MousePos;
		MouseButton m_Button;
	};

	class MouseButtonReleasedEvent : public EventBase<MouseEvent, EventType::MouseButtonReleasd, EventCategory::Input | EventCategory::Mouse>
	{
	public:
		MouseButtonReleasedEvent(const glm::ivec2& mousePos, MouseButton button)
			: m_MousePos(mousePos), m_Button(button)
		{}

		virtual const glm::ivec2& GetMousePos() const override { return m_MousePos; }
		virtual int GetX() const override { return m_MousePos.x; }
		virtual int GetY() const override { return m_MousePos.y; }
		virtual MouseButton GetButton() const override { return m_Button; }

		std::string ToString() const override { return fmt::format("{}, Pos: {}, Button: {}", GetName(), m_MousePos, ::Shark::ToString(m_Button)); }

	private:
		glm::ivec2 m_MousePos;
		MouseButton m_Button;
	};

	class MouseButtonDoubleClickedEvent : public EventBase<MouseEvent, EventType::MouseButtonDoubleClicked, EventCategory::Input | EventCategory::Mouse>
	{
	public:
		MouseButtonDoubleClickedEvent(const glm::ivec2& mousePos, MouseButton button)
			: m_MousePos(mousePos), m_Button(button)
		{}

		virtual const glm::ivec2& GetMousePos() const override { return m_MousePos; }
		virtual int GetX() const override { return m_MousePos.x; }
		virtual int GetY() const override { return m_MousePos.y; }
		virtual MouseButton GetButton() const override { return m_Button; }

		std::string ToString() const override { return fmt::format("{}, Pos: {}, Button: {}", GetName(), m_MousePos, ::Shark::ToString(m_Button)); }

	private:
		glm::ivec2 m_MousePos;
		MouseButton m_Button;
	};

	class MouseScrolledEvent : public EventBase<MouseEvent, EventType::MouseScrolled, EventCategory::Input | EventCategory::Mouse>
	{
	public:
		MouseScrolledEvent(const glm::ivec2& mousePos, float delta)
			: m_MousePos(mousePos), m_Delta(delta)
		{}

		virtual const glm::ivec2& GetMousePos() const override { return m_MousePos; }
		virtual int GetX() const override { return m_MousePos.x; }
		virtual int GetY() const override { return m_MousePos.y; }
		virtual MouseButton GetButton() const override { return MouseButton::None; }
		float GetDelta() const { return m_Delta; }

		std::string ToString() const override { return fmt::format("{}, Pos: {}, Delta: {}", GetName(), m_MousePos, m_Delta); }

	private:
		glm::ivec2 m_MousePos;
		float m_Delta;
	};

}