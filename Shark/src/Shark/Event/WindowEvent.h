#pragma once
#include "Shark/Core.h"
#include "Event.h"

namespace Shark {

	class SHARK_API WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent( int ExitCode )
			:
			ExitCode( ExitCode )
		{}
		int GetExitCode() const { return ExitCode; }

		SK_GET_CATEGORY_FLAGS_FUNC( EventCategoryWindow )
		SK_EVENT_FUNCTIONS( WindowClose )
	private:
		int ExitCode;
	};

	class SHARK_API WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent( unsigned int width,unsigned int height )
			:
			width( width ),
			height( height )
		{}
		unsigned int GetWidth() const { return width; }
		unsigned int GetHeight() const { return height; }

		SK_GET_CATEGORY_FLAGS_FUNC( EventCategoryWindow )
		SK_EVENT_FUNCTIONS( WindowResize )
	private:
		unsigned int width;
		unsigned int height;
	};

	class SHARK_API WindowMoveEvent : public Event
	{
	public:
		WindowMoveEvent( int x,int y )
			:
			x( x ),
			y( y )
		{}
		int GetX() const { return x; }
		int GetY() const { return y; }

		SK_GET_CATEGORY_FLAGS_FUNC( EventCategoryWindow )
		SK_EVENT_FUNCTIONS( WindowMove )
	private:
		int x;
		int y;
	};

	class SHARK_API WindowFocusEvent : public Event
	{
	public:
		SK_GET_CATEGORY_FLAGS_FUNC( EventCategoryWindow )
		SK_EVENT_FUNCTIONS( WindowFocus )
	};

	class SHARK_API WindowLostFocusEvent : public Event
	{
	public:
		SK_GET_CATEGORY_FLAGS_FUNC( EventCategoryWindow )
		SK_EVENT_FUNCTIONS( WindowLostFocus )
	};

}