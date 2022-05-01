#pragma once
#include "Shark/Core/Base.h"
#include "Event.h"
#include "Shark/Core/MouseButtons.h"

namespace Shark {

	class MouseMoveEvent : public EventBase<EventTypes::MouseMove, EventCategoryInput | EventCategoryMouse>
	{
	public:
		MouseMoveEvent(const glm::ivec2& mousepos)
			: m_MousePos(mousepos)
		{}

		const glm::ivec2& GetMousePos() const { return m_MousePos; }
		int GetX() const { return m_MousePos.x; }
		int GetY() const { return m_MousePos.y; }

		std::string ToString() const override { return fmt::format("{}, Pos: {}", GetName(), m_MousePos); }

	private:
		glm::ivec2 m_MousePos;
	};

	class MouseButtonPressedEvent : public EventBase<EventTypes::MouseButtonPressed, EventCategoryInput | EventCategoryMouse>
	{
	public:
		MouseButtonPressedEvent(const glm::ivec2& mousePos, MouseButton::Type button)
			: m_MousePos(mousePos), m_Button(button)
		{}

		const glm::ivec2& GetMousePos() const { return m_MousePos; }
		int GetX() const { return m_MousePos.x; }
		int GetY() const { return m_MousePos.y; }
		MouseButton::Type GetButton() { return m_Button; }

		std::string ToString() const override { return fmt::format("{}, Pos: {}, Button: {}", GetName(), m_MousePos, ::Shark::ToString(m_Button)); }

	private:
		glm::ivec2 m_MousePos;
		MouseButton::Type m_Button;
	};

	class MouseButtonReleasedEvent : public EventBase<EventTypes::MouseButtonReleasd, EventCategoryInput | EventCategoryMouse>
	{
	public:
		MouseButtonReleasedEvent(const glm::ivec2& mousePos, MouseButton::Type button)
			: m_MousePos(mousePos), m_Button(button)
		{}

		const glm::ivec2& GetMousePos() const { return m_MousePos; }
		int GetX() const { return m_MousePos.x; }
		int GetY() const { return m_MousePos.y; }
		MouseButton::Type GetButton() { return m_Button; }

		std::string ToString() const override { return fmt::format("{}, Pos: {}, Button: {}", GetName(), m_MousePos, ::Shark::ToString(m_Button)); }

	private:
		glm::ivec2 m_MousePos;
		MouseButton::Type m_Button;
	};

	class MouseButtonDoubleClicked : public EventBase<EventTypes::MouseButtonDoubleClicked, EventCategoryInput | EventCategoryMouse>
	{
	public:
		MouseButtonDoubleClicked(const glm::ivec2& mousePos, MouseButton::Type button)
			: m_MousePos(mousePos), m_Button(button)
		{}

		const glm::ivec2& GetMousePos() const { return m_MousePos; }
		int GetX() const { return m_MousePos.x; }
		int GetY() const { return m_MousePos.y; }
		MouseButton::Type GetButton() { return m_Button; }

		std::string ToString() const override { return fmt::format("{}, Pos: {}, Button: {}", GetName(), m_MousePos, ::Shark::ToString(m_Button)); }

	private:
		glm::ivec2 m_MousePos;
		MouseButton::Type m_Button;
	};

	class MouseScrolledEvent : public EventBase<EventTypes::MouseScrolled, EventCategoryInput | EventCategoryMouse>
	{
	public:
		MouseScrolledEvent(const glm::ivec2& mousePos, float delta)
			: m_MousePos(mousePos), m_Delta(delta)
		{}

		const glm::ivec2& GetMousePos() const { return m_MousePos; }
		int GetX() const { return m_MousePos.x; }
		int GetY() const { return m_MousePos.y; }
		float GetDelta() const { return m_Delta; }

		std::string ToString() const override { return fmt::format("{}, Pos: {}, Delta: {}", GetName(), m_MousePos, m_Delta); }

	private:
		glm::ivec2 m_MousePos;
		float m_Delta;
	};

}