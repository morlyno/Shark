#pragma once
#pragma warning(push)
#pragma warning(disable : 26812)
#include "skpch.h"
#include "Shark/Core.h"

namespace Shark {

	enum class EventTypes
	{
		None = 0,
		WindowClose,WindowResize,WindowMove,WindowFocus,WindowLostFocus,
		MouseMove,MousButtonPressed,MouseButtonReleasd,MouseScrolled,
		KeyPressed,KeyReleased,KeyCharacter
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryWindow = BIT( 0 ),
		EventCategoryInput = BIT( 1 ),
		EventCategoryMouse = BIT( 2 ),
		EventCategoryKeyboard = BIT( 3 ),
	};

	#define SK_EVENT_FUNCTIONS(type)	static EventTypes GetStaticType() { return EventTypes::##type; } \
										virtual EventTypes GetEventType() const override { return GetStaticType(); } \
										virtual const char* GetName() const override { return #type; }

	#define SK_GET_CATEGORY_FLAGS_FUNC(category) unsigned int GetEventCategoryFlags() const override { return category; }

	class SHARK_API Event
	{
	public:
		virtual EventTypes GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual std::string ToString() const { return GetName(); }
		virtual unsigned int GetEventCategoryFlags() const = 0;
		bool IsInCategory( EventCategory category ) const
		{
			return GetEventCategoryFlags() & category;
		}
	private:
		bool Handled = false;
	};

	inline std::ostream& operator<<( std::ostream& os,const Event& e )
	{
		return os << e.ToString();
	}

}

#pragma warning(pop)