#pragma once
#include "Shark/Core.h"
#include "Event.h"

namespace Shark {

	class SHARK_API WindowEvent : public Event
	{
	public:
		static EventTypes GetStaticType() { return EventTypes::WindowEventBase; }

		SK_GET_CATEGORY_FLAGS_FUNC( EventCategoryWindow )
	};

	class SHARK_API WindowCloseEvent : public WindowEvent
	{
	public:
		WindowCloseEvent( int ExitCode )
			:
			ExitCode( ExitCode )
		{}
		int GetExitCode() const { return ExitCode; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << ExitCode;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS( WindowClose )
	private:
		int ExitCode;
	};

	class SHARK_API WindowResizeEvent : public WindowEvent
	{
	public:
		WindowResizeEvent( unsigned int width,unsigned int height )
			:
			width( width ),
			height( height )
		{}
		unsigned int GetWidth() const { return width; }
		unsigned int GetHeight() const { return height; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << width << " , " << height;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS( WindowResize )
	private:
		unsigned int width;
		unsigned int height;
	};

	class SHARK_API WindowMoveEvent : public WindowEvent
	{
	public:
		WindowMoveEvent( int x,int y )
			:
			x( x ),
			y( y )
		{}
		int GetX() const { return x; }
		int GetY() const { return y; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << x << " , " << y;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS( WindowMove )
	private:
		int x;
		int y;
	};

	class SHARK_API WindowFocusEvent : public WindowEvent
	{
	public:
		WindowFocusEvent( int x,int y )
			:
			x( x ),
			y( y )
		{}
		int GetX() const { return x; }
		int GetY() const { return y; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << x << " , " << y;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS( WindowFocus )
	private:
		int x;
		int y;
	};

	class SHARK_API WindowLostFocusEvent : public WindowEvent
	{
	public:
		SK_EVENT_FUNCTIONS( WindowLostFocus )
	};

	class SHARK_API WindowMaximizedEvent : public WindowEvent
	{
	public:
		WindowMaximizedEvent( unsigned int width,unsigned int height )
			:
			width( width ),
			height( height )
		{}
		unsigned int GetWidth() const { return width; }
		unsigned int GetHeight() const { return height; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << width << " , " << height;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS( WindowMaximized )
	private:
		unsigned int width;
		unsigned int height;
	};

	class SHARK_API WindowMinimizedEvent : public WindowEvent
	{
	public:
		SK_EVENT_FUNCTIONS( WindowMinimized )
	};

}