#pragma once
#include "Shark/Core/Base.h"
#include "Event.h"

namespace Shark {

	class WindowCloseEvent : public EventBase<Event, EventType::WindowClose, EventCategory::Window>
	{
	public:
		WindowCloseEvent() = default;
	};

	class WindowResizeEvent : public EventBase<Event, EventType::WindowResize, EventCategory::Window>
	{
	public:
		enum class State { Resize = 0, Maximized, Minimized };
		std::string StateToString(State state) const
		{
			switch (state)
			{
				case State::Resize:    return "Resize";
				case State::Maximized: return "Maximized";
				case State::Minimized: return "Minimized";
			}
			SK_CORE_ASSERT(false, "Unkown State");
			return "Unkown";
		}
	public:
		WindowResizeEvent(uint32_t width, uint32_t height, State state)
			: m_Width(width), m_Height(height), m_State(state)
		{}

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
		State GetState() const { return m_State; }
		bool IsMinimized() const { return m_State == State::Minimized; }
		bool IsMaximized() const { return m_State == State::Maximized; }

		std::string ToString() const override { return fmt::format("{}, Size: [{}, {}], State: {}", GetName(), m_Width, m_Height, StateToString(m_State)); }

	private:
		uint32_t m_Width;
		uint32_t m_Height;
		State m_State;
	};

	class WindowMoveEvent : public EventBase<Event, EventType::WindowMove, EventCategory::Window>
	{
	public:
		WindowMoveEvent(int x, int y)
			: m_X(x), m_Y(y)
		{}

		int GetX() const { return m_X; }
		int GetY() const { return m_Y; }

		std::string ToString() const override { return fmt::format("{}, Pos: [{}, {}]", GetName(), m_X, m_Y); }

	private:
		int m_X, m_Y;
	};

	class WindowFocusEvent : public EventBase<Event, EventType::WindowFocus, EventCategory::Window>
	{
	};

	class WindowLostFocusEvent : public EventBase<Event, EventType::WindowLostFocus, EventCategory::Window>
	{
	};

	class WindowDropEvent : public EventBase<Event, EventType::WindowDrop, EventCategory::Window>
	{
	public:
		WindowDropEvent(const std::vector<std::filesystem::path>& paths)
			: m_Paths(paths)
		{}

		WindowDropEvent(std::vector<std::filesystem::path>&& paths)
			: m_Paths(std::move(paths))
		{}
		
		const std::vector<std::filesystem::path>& GetPaths() const { return m_Paths; }

		std::string ToString() const override { return fmt::format("{}\n\t- {}", GetName(), fmt::join(m_Paths, "\n\t- ")); }

	private:
		std::vector<std::filesystem::path> m_Paths;
	};

}