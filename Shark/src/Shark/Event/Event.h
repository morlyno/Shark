#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	enum class EventTypes
	{
		None = 0,
		WindowClose, WindowResize, WindowMove, WindowFocus, WindowLostFocus,
		MouseMove, MouseButtonPressed, MouseButtonReleasd, MouseButtonDoubleClicked, MouseScrolled,
		KeyPressed, KeyReleased, KeyCharacter,
		ApplicationClosed, SceneChanged, ProjectChanged
	};

	inline std::string EventTypesToString(EventTypes eventType)
	{
		switch (eventType)
		{
			case EventTypes::None:                       return "None";
			case EventTypes::WindowClose:                return "WindowClose";
			case EventTypes::WindowResize:		         return "WindowResize";
			case EventTypes::WindowMove:		         return "WindowMove";
			case EventTypes::WindowFocus:		         return "WindowFocus";
			case EventTypes::WindowLostFocus:	         return "WindowLostFocus";
			case EventTypes::MouseMove:			         return "MouseMove";
			case EventTypes::MouseButtonPressed:         return "MouseButtonPressed";
			case EventTypes::MouseButtonReleasd:         return "MouseButtonReleasd";
			case EventTypes::MouseButtonDoubleClicked:   return "MouseButtonDoubleClicked";
			case EventTypes::MouseScrolled:		         return "MouseScrolled";
			case EventTypes::KeyPressed:		         return "KeyPressed";
			case EventTypes::KeyReleased:		         return "KeyReleased";
			case EventTypes::KeyCharacter:		         return "KeyCharacter";
			case EventTypes::ApplicationClosed:	         return "ApplicationClosed";
			case EventTypes::SceneChanged:	             return "SceneChanged";
			case EventTypes::ProjectChanged:	         return "ProjectChanged";
		}
		SK_CORE_ASSERT(false, "Unkown Event Type");
		return "Unkown";
	}

	enum EventCategory_ : uint32_t
	{
		EventCategoryNone         = 0,
		EventCategoryWindow       = BIT(0),
		EventCategoryInput        = BIT(1),
		EventCategoryMouse        = BIT(2),
		EventCategoryKeyboard     = BIT(3),
		EventCategoryApplication  = BIT(4),
	};
	using EventCategory = uint32_t;

	template<EventTypes Type, uint32_t Category>
	class EventBase : public Event
	{
	public:
		static constexpr EventTypes GetStaticType() { return Type; }
		virtual EventTypes GetEventType() const override { return GetStaticType(); }
		virtual std::string GetName() const override { return EventTypesToString(Type); }

		static constexpr uint32_t GetStaticEventCategoryFlags() { return Category; }
		virtual uint32_t GetEventCategoryFlags() const override { return GetStaticEventCategoryFlags(); }
	};

	class Event
	{
	public:
		virtual ~Event() = default;

		virtual EventTypes GetEventType() const = 0;
		virtual std::string GetName() const = 0;
		virtual std::string ToString() const { return GetName(); }
		virtual uint32_t GetEventCategoryFlags() const = 0;
		bool IsInCategory(EventCategory category) const { return GetEventCategoryFlags() & category; }

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
