#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowMove, WindowFocus, WindowLostFocus,
		MouseMoved, MouseButtonPressed, MouseButtonReleasd, MouseButtonDoubleClicked, MouseScrolled,
		KeyPressed, KeyReleased, KeyCharacter,
		ApplicationClosed, SceneChanged, ProjectChanged
	};

	inline std::string EventTypesToString(EventType eventType)
	{
		switch (eventType)
		{
			case EventType::None:                      return "None";
			case EventType::WindowClose:               return "WindowClose";
			case EventType::WindowResize:		       return "WindowResize";
			case EventType::WindowMove:		           return "WindowMove";
			case EventType::WindowFocus:		       return "WindowFocus";
			case EventType::WindowLostFocus:	       return "WindowLostFocus";
			case EventType::MouseMoved:			       return "MouseMoved";
			case EventType::MouseButtonPressed:        return "MouseButtonPressed";
			case EventType::MouseButtonReleasd:        return "MouseButtonReleasd";
			case EventType::MouseButtonDoubleClicked:  return "MouseButtonDoubleClicked";
			case EventType::MouseScrolled:		       return "MouseScrolled";
			case EventType::KeyPressed:		           return "KeyPressed";
			case EventType::KeyReleased:		       return "KeyReleased";
			case EventType::KeyCharacter:		       return "KeyCharacter";
			case EventType::ApplicationClosed:	       return "ApplicationClosed";
			case EventType::SceneChanged:	           return "SceneChanged";
			case EventType::ProjectChanged:	           return "ProjectChanged";
		}
		SK_CORE_ASSERT(false, "Unkown Event Type");
		return "Unkown";
	}

	namespace EventCategory {
		enum Type : uint16_t
		{
			None         = 0,
			Window       = BIT(0),
			Input        = BIT(1),
			Mouse        = BIT(2),
			Keyboard     = BIT(3),
			Application  = BIT(4)
		};
		using Flags = uint16_t;
	}

	template<EventType Type, EventCategory::Flags Category>
	class EventBase : public Event
	{
	public:
		static constexpr EventType GetStaticType() { return Type; }
		virtual EventType GetEventType() const override { return GetStaticType(); }
		virtual std::string GetName() const override { return EventTypesToString(Type); }

		static constexpr EventCategory::Flags GetStaticEventCategoryFlags() { return Category; }
		virtual EventCategory::Flags GetEventCategoryFlags() const override { return GetStaticEventCategoryFlags(); }
	};

	namespace EventStatus
	{
		enum Type : uint16_t
		{
			None = 0,
			BlockedByImGui = BIT(0)
		};
		using Flags = uint16_t;
	};

	class Event
	{
	public:
		virtual ~Event() = default;

		virtual EventType GetEventType() const = 0;
		virtual std::string GetName() const = 0;
		virtual std::string ToString() const { return GetName(); }
		virtual EventCategory::Flags GetEventCategoryFlags() const = 0;
		bool IsInCategory(EventCategory::Flags category) const { return GetEventCategoryFlags() & category; }

	public:
		bool Handled = false;
		EventStatus::Flags Status = EventStatus::None;
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
			if (!m_Event.Handled && (T::GetStaticType() == m_Event.GetEventType()))
			{
				m_Event.Handled |= func(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}
		template<typename T, typename Func>
		bool DispachEventAlways(const Func& func)
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

}

template<typename Event>
struct fmt::formatter<Event, char, std::enable_if_t<std::is_same_v<Event, Shark::Event> || std::is_base_of_v<Shark::Event, Event>>>
{	
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
	{
		return ctx.end();
	}

	template<typename FormatContext>
	auto format(const Shark::Event& event, FormatContext& ctx) -> decltype(ctx.out())
	{
		return format_to(ctx.out(), event.ToString());
	}
};
