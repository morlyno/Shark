#pragma once
#include "Shark/Core/Core.h"
#include "Event.h"

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

		static EventTypes GetStaticType() { return EventTypes::MouseEventBase; }

		SK_GET_CATEGORY_FLAGS_FUNC( EventCategoryInput | EventCategoryMouse )
	protected:
		MouseEvent( int x,int y )
			:
			x( x ),
			y( y )
		{}
		int x;
		int y;
	};

	class MouseMoveEvent : public MouseEvent
	{
	public:
		MouseMoveEvent( int x,int y )
			:
			MouseEvent( x,y )
		{}
		SK_EVENT_FUNCTIONS( MouseMove )
	};

	class MouseLeftPressedEvent : public MouseEvent
	{
	public:
		MouseLeftPressedEvent( int x,int y )
			:
			MouseEvent( x,y )
		{}
		SK_EVENT_FUNCTIONS( MousButtonPressed )
	};

	class MouseLeftReleasedEvent : public MouseEvent
	{
	public:
		MouseLeftReleasedEvent( int x,int y )
			:
			MouseEvent( x,y )
		{}
		SK_EVENT_FUNCTIONS( MouseButtonReleasd )
	};

	class MouseRightPressedEvent : public MouseEvent
	{
	public:
		MouseRightPressedEvent( int x,int y )
			:
			MouseEvent( x,y )
		{}
		SK_EVENT_FUNCTIONS( MousButtonPressed )
	};

	class MouseRightReleasedEvent : public MouseEvent
	{
	public:
		MouseRightReleasedEvent( int x,int y )
			:
			MouseEvent( x,y )
		{}
		SK_EVENT_FUNCTIONS( MouseButtonReleasd )
	};

	class MouseScrolledEvent : public MouseEvent
	{
	public:
		MouseScrolledEvent( int x,int y,int delta )
			:
			MouseEvent( x,y ),
			delta( delta )
		{}

		int GetDelta() const { return delta; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << x << ", " << y << " Delta: " << delta;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS( MouseButtonReleasd )
	private:
		int delta;
	};

}