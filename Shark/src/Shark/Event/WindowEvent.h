#pragma once
#include "Shark/Core/Base.h"
#include "Event.h"

namespace Shark {

	class WindowEvent : public Event
	{
	public:
		SK_GET_CATEGORY_FLAGS_FUNC(EventCategoryWindow)
	};

	class WindowCloseEvent : public WindowEvent
	{
	public:
		SK_EVENT_FUNCTIONS(WindowClose)
	};

	class WindowResizeEvent : public WindowEvent
	{
	public:
		enum class State { None = 0, Maximized, Minimized };
	public:
		WindowResizeEvent(uint32_t width, uint32_t height, State state)
			:
			m_Width(width),
			m_Height(height),
			m_State(state)
		{}
		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
		State GetState() const { return m_State; }
		bool IsMinimized() const { return m_State == State::Minimized; }
		bool IsMaximized() const { return m_State == State::Maximized; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << m_Width << " , " << m_Height << ", " << (uint32_t)m_State;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS(WindowResize)
	private:
		uint32_t m_Width;
		uint32_t m_Height;
		State m_State;
	};

	class WindowMoveEvent : public WindowEvent
	{
	public:
		WindowMoveEvent(int x, int y)
			:
			x(x),
			y(y)
		{}
		int GetX() const { return x; }
		int GetY() const { return y; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << x << " , " << y;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS(WindowMove)
	private:
		int x;
		int y;
	};

	class WindowFocusEvent : public WindowEvent
	{
	public:
		WindowFocusEvent(int x, int y)
			:
			x(x),
			y(y)
		{}
		int GetX() const { return x; }
		int GetY() const { return y; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << x << " , " << y;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS(WindowFocus)
	private:
		int x;
		int y;
	};

	class WindowLostFocusEvent : public WindowEvent
	{
	public:
		SK_EVENT_FUNCTIONS(WindowLostFocus)
	};

}