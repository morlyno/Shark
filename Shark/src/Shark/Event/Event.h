#pragma once

#include "Shark/Core/Base.h"
#include <magic_enum.hpp>

namespace Shark {

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowMaximized, WindowMinimized, WindowMove, WindowFocus, WindowLostFocus, WindowDrop,
		MouseMoved, MouseButtonPressed, MouseButtonReleasd, MouseButtonDoubleClicked, MouseScrolled,
		KeyPressed, KeyReleased, KeyCharacter,
		ApplicationClosed, AssetReloaded
	};

	enum class EventCategory : uint16_t
	{
		None         = 0,
		Window       = BIT(0),
		Input        = BIT(1),
		Mouse        = BIT(2),
		Keyboard     = BIT(3),
		Application  = BIT(4)
	};

	class Event
	{
	public:
		virtual ~Event() = default;

		virtual EventType GetEventType() const = 0;
		virtual std::string_view GetName() const = 0;
		virtual std::string ToString() const { return std::string(GetName()); }
		virtual EventCategory GetEventCategoryFlags() const = 0;
		bool IsInCategory(EventCategory category) const { return (GetEventCategoryFlags() & category) == category; }

		template<typename TEvent>
		TEvent& As()
		{
			SK_CORE_ASSERT(TEvent::GetStaticType() == GetEventType());
			return (TEvent&)*this;
		}

	public:
		bool Handled = false;
	};

	template<EventType Type, EventCategory Category>
	class EventBase : public Event
	{
	public:
		static constexpr EventType GetStaticType() { return Type; }
		virtual EventType GetEventType() const override { return GetStaticType(); }
		virtual std::string_view GetName() const override { return magic_enum::enum_name(Type); }

		static constexpr EventCategory GetStaticEventCategoryFlags() { return Category; }
		virtual EventCategory GetEventCategoryFlags() const override { return GetStaticEventCategoryFlags(); }
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
				m_Event.Handled |= func((T&)m_Event);
				return true;
			}
			return false;
		}

	private:
		Event& m_Event;
	};

}

template<typename TEvent>
	requires std::is_same_v<TEvent, Shark::Event> || std::is_base_of_v<Shark::Event, char>
struct fmt::formatter<TEvent>
{	
	constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator
	{
		return ctx.end();
	}

	template<typename FormatContext>
	auto format(const TEvent& event, FormatContext& ctx) const -> FormatContext::iterator
	{
		return fmt::format_to(ctx.out(), "{0}", event.ToString());
	}
};

template <>
struct ::magic_enum::customize::enum_range<Shark::EventCategory> {
	static constexpr bool is_flags = true;
};
