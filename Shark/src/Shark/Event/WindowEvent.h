#pragma once
#include "Shark/Core/Base.h"
#include "Event.h"

namespace Shark {

	class WindowEvent : public Event
	{
	public:
		static EventTypes GetStaticType() { return EventTypes::WindowEventBase; }

		SK_GET_CATEGORY_FLAGS_FUNC(EventCategoryWindow)
	};

	class WindowCloseEvent : public WindowEvent
	{
	public:
		WindowCloseEvent(int ExitCode)
			:
			ExitCode(ExitCode)
		{}
		int GetExitCode() const { return ExitCode; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << ExitCode;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS(WindowClose)
	private:
		int ExitCode;
	};

	class WindowResizeEvent : public WindowEvent
	{
	public:
		WindowResizeEvent(unsigned int width, unsigned int height)
			:
			width(width),
			height(height)
		{}
		unsigned int GetWidth() const { return width; }
		unsigned int GetHeight() const { return height; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << width << " , " << height;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS(WindowResize)
	private:
		unsigned int width;
		unsigned int height;
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

	class WindowMaximizedEvent : public WindowEvent
	{
	public:
		WindowMaximizedEvent(unsigned int width, unsigned int height)
			:
			width(width),
			height(height)
		{}
		unsigned int GetWidth() const { return width; }
		unsigned int GetHeight() const { return height; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << width << " , " << height;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS(WindowMaximized)
	private:
		unsigned int width;
		unsigned int height;
	};

	class WindowMinimizedEvent : public WindowEvent
	{
	public:
		SK_EVENT_FUNCTIONS(WindowMinimized)
	};

}