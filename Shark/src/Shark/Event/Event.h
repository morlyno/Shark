#pragma once
#include "Shark/Core/Base.h"

#include <sstream>


#define SK_EVENT_FUNCTIONS(type) static constexpr ::Shark::EventTypes GetStaticType() { return ::Shark::EventTypes::##type; } \
								 virtual ::Shark::EventTypes GetEventType() const override { return GetStaticType(); } \
								 virtual const char* GetName() const override { return #type; }

#define SK_GET_CATEGORY_FLAGS_FUNC(category) static constexpr unsigned int GetStaticEventCategoryFlags() { return category; } \
											 unsigned int GetEventCategoryFlags() const override { return GetStaticEventCategoryFlags(); }

namespace Shark {

	enum class EventTypes
	{
		None = 0,
		WindowClose, WindowResize, WindowMove, WindowFocus, WindowLostFocus, WindowMinimized, WindowMaximized,
		MouseMove, MouseButtonPressed, MouseButtonReleasd, MouseScrolled,
		KeyPressed, KeyReleased, KeyCharacter,
		ApplicationClosed,
		SelectionChanged
	};

	enum EventCategory_ : uint32_t
	{
		None                      = 0,
		EventCategoryWindow       = SK_BIT(0),
		EventCategoryInput        = SK_BIT(1),
		EventCategoryMouse        = SK_BIT(2),
		EventCategoryKeyboard     = SK_BIT(3),
		EventCategoryApplication  = SK_BIT(4),
	};
	using EventCategory = uint32_t;

	class Event
	{
	public:
		virtual ~Event() = default;

		virtual EventTypes GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual std::string ToString() const { return GetName(); }
		virtual unsigned int GetEventCategoryFlags() const = 0;
		bool IsInCategory(EventCategory category) const { return GetEventCategoryFlags() & category; }

		static void Distribute(Event& event);
	public:
		bool Handled = false;
	};

	class EventDispacher
	{
	public:
		EventDispacher(Event& event)
			: m_Event(event)
		{}

		template<typename T, typename Func>
		bool DispachEvent(const Func& func)
		{
			if (T::GetStaticType() == m_Event.GetEventType())
			{
				m_Event.Handled |= func(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}
	private:
		Event& m_Event;
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}

}
