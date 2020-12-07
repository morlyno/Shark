#pragma once
#pragma warning(push)
#pragma warning(disable : 26812)
#include "skpch.h"
#include "Shark/Core/Core.h"

namespace Shark {

	enum class EventTypes
	{
		None = 0,
		WindowClose,WindowResize,WindowMove,WindowFocus,WindowLostFocus,WindowMinimized,WindowMaximized,WindowEventBase,
		MouseMove,MouseButtonPressed,MouseButtonReleasd,MouseScrolled,MouseEventBase,
		KeyPressed,KeyReleased,KeyCharacter,KeyEventBase
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryWindow = SK_BIT( 0 ),
		EventCategoryInput = SK_BIT( 1 ),
		EventCategoryMouse = SK_BIT( 2 ),
		EventCategoryKeyboard = SK_BIT( 3 ),
	};
	typedef int EventCategory_t;

	#define SK_EVENT_FUNCTIONS(type)	static EventTypes GetStaticType() { return EventTypes::##type; } \
										virtual EventTypes GetEventType() const override { return GetStaticType(); } \
										virtual const char* GetName() const override { return #type; }

	#define SK_GET_CATEGORY_FLAGS_FUNC(category)	static unsigned int GetStaticEventCategoryFlags() { return category; } \
													unsigned int GetEventCategoryFlags() const override { return category; }

	class Event
	{
		friend class EventDispacher;
	public:
		virtual EventTypes GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual std::string ToString() const { return GetName(); }
		virtual unsigned int GetEventCategoryFlags() const = 0;
		bool IsInCategory( EventCategory category ) const
		{
			return GetEventCategoryFlags() & category;
		}
		bool IsInCategory( EventCategory_t category ) const
		{
			return GetEventCategoryFlags() & category;
		}
	private:
		bool Handled = false;
	};

	class EventDispacher
	{
		template<typename T>
		using EventFunc = std::function<bool( T& )>;
	public:
		EventDispacher( Event& e )
			:
			e( e )
		{}
		template<typename T>
		bool DispachEvent( EventFunc<T> func )
		{
			if ( T::GetStaticType() == e.GetEventType() )
			{
				e.Handled = func( *reinterpret_cast<T*>( &e ) );
				return true;
			}
			return false;
		}
		template<typename T>
		bool DispachEventCategory( EventFunc<T> func )
		{
			if ( T::GetStaticEventCategoryFlags() & e.GetEventCategoryFlags() )
			{
				e.Handled = func( *reinterpret_cast<T*>( &e ) );
				return true;
			}
			return false;
		}
	private:
		Event& e;
	};

	inline std::ostream& operator<<( std::ostream& os,const Event& e )
	{
		return os << e.ToString();
	}

}

#pragma warning(pop)