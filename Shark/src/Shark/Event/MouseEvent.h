#pragma once
#include "Shark/Core/Base.h"
#include "Event.h"
#include "Shark/Core/MouseCodes.h"

namespace Shark {

	class MouseEvent : public Event
	{
	public:
		int GetX() const { return x; }
		int GetY() const { return y; }

		virtual std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << x << ", " << y;
			return oss.str();
		}

		SK_GET_CATEGORY_FLAGS_FUNC(EventCategoryInput | EventCategoryMouse)
	protected:
		MouseEvent(int x, int y)
			:
			x(x),
			y(y)
		{}
		int x;
		int y;
	};

	class MouseMoveEvent : public MouseEvent
	{
	public:
		MouseMoveEvent(int x, int y)
			:
			MouseEvent(x, y)
		{}
		SK_EVENT_FUNCTIONS(MouseMove)
	};

	class MousePressedEvent : public MouseEvent
	{
	public:
		MousePressedEvent(int x, int y, MouseCode button)
			:
			MouseEvent(x, y),
			m_Button(button)
		{}
		inline MouseCode GetButton() { return m_Button; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << x << ", " << y << " Button: " << m_Button;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS(MouseButtonPressed)
	private:
		MouseCode m_Button;
	};

	class MouseReleasedEvent : public MouseEvent
	{
	public:
		MouseReleasedEvent(int x, int y, MouseCode button)
			:
			MouseEvent(x, y),
			m_Button(button)
		{}
		inline int GetButton() { return m_Button; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << x << ", " << y << " Button: " << m_Button;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS(MouseButtonReleasd)
	private:
		MouseCode m_Button;
	};

	class MouseScrolledEvent : public MouseEvent
	{
	public:
		MouseScrolledEvent(int x, int y, int delta)
			:
			MouseEvent(x, y),
			m_Delta(delta)
		{}

		inline int GetDelta() const { return m_Delta; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << x << ", " << y << " Delta: " << m_Delta;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS(MouseScrolled)
	private:
		int m_Delta;
	};

}