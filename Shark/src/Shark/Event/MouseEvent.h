#pragma once
#include "Shark/Core/Core.h"
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

	class MousePressedEvent : public MouseEvent
	{
	public:
		MousePressedEvent( int x,int y,MouseCode button )
			:
			MouseEvent( x,y ),
			button( button )
		{}
		inline int GetButton() { return button; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << x << ", " << y << " Button: " << button;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS( MouseButtonPressed )
	private:
		MouseCode button;
	};

	class MouseReleasedEvent : public MouseEvent
	{
	public:
		MouseReleasedEvent( int x,int y,MouseCode button )
			:
			MouseEvent( x,y ),
			button( button )
		{}
		inline int GetButton() { return button; }

		std::string ToString() const override
		{
			std::ostringstream oss;
			oss << GetName() << " " << x << ", " << y << " Button: " << button;
			return oss.str();
		}

		SK_EVENT_FUNCTIONS( MouseButtonReleasd )
	private:
		MouseCode button;
	};

	class MouseScrolledEvent : public MouseEvent
	{
	public:
		MouseScrolledEvent( int x,int y,int delta )
			:
			MouseEvent( x,y ),
			delta( delta )
		{}

		inline int GetDelta() const { return delta; }

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