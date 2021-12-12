#pragma once
#include "Shark/Core/Base.h"
#include "Event.h"
#include "Shark/Core/MouseCodes.h"

namespace Shark {

	class MouseMoveEvent : public EventBase<EventTypes::MouseMove, EventCategoryInput | EventCategoryMouse>
	{
	public:
		MouseMoveEvent(int x, int y)
			: m_X(x), m_Y(y)
		{}

		int GetX() const { return m_X; }
		int GetY() const { return m_Y; }

		std::string ToString() const override { return fmt::format("{}, Pos: [{}, {}]", GetName(), m_X, m_Y); }

	private:
		int m_X, m_Y;
	};

	class MousePressedEvent : public EventBase<EventTypes::MouseButtonPressed, EventCategoryInput | EventCategoryMouse>
	{
	public:
		MousePressedEvent(int x, int y, MouseCode button)
			: m_X(x), m_Y(y), m_Button(button)
		{}

		int GetX() const { return m_X; }
		int GetY() const { return m_Y; }
		MouseCode GetButton() { return m_Button; }

		std::string ToString() const override { return fmt::format("{}, Pos: [{}, {}], Button: {}", GetName(), m_X, m_Y, m_Button); }

	private:
		int m_X, m_Y;
		MouseCode m_Button;
	};

	class MouseReleasedEvent : public EventBase<EventTypes::MouseButtonReleasd, EventCategoryInput | EventCategoryMouse>
	{
	public:
		MouseReleasedEvent(int x, int y, MouseCode button)
			: m_X(x), m_Y(y), m_Button(button)
		{}

		int GetX() const { return m_X; }
		int GetY() const { return m_Y; }
		MouseCode GetButton() { return m_Button; }

		std::string ToString() const override { return fmt::format("{}, Pos: [{}, {}], Button: {}", GetName(), m_X, m_Y, m_Button); }

	private:
		int m_X, m_Y;
		MouseCode m_Button;
	};

	class MouseScrolledEvent : public EventBase<EventTypes::MouseScrolled, EventCategoryInput | EventCategoryMouse>
	{
	public:
		MouseScrolledEvent(int x, int y, int delta)
			: m_X(x), m_Y(y), m_Delta(delta)
		{}

		int GetX() const { return m_X; }
		int GetY() const { return m_Y; }
		int GetDelta() const { return m_Delta; }

		std::string ToString() const override { return fmt::format("{}, Pos: [{}, {}], Delta: {}", GetName(), m_X, m_Y, m_Delta); }

	private:
		int m_X, m_Y;
		int m_Delta;
	};

}