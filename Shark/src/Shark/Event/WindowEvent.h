#pragma once
#include "Shark/Core/Base.h"
#include "Event.h"

namespace Shark {

	class WindowCloseEvent : public EventBase<EventType::WindowClose, EventCategory::Window>
	{
	};

	class WindowResizeEvent : public EventBase<EventType::WindowResize, EventCategory::Window>
	{
	public:
		WindowResizeEvent(uint32_t width, uint32_t height)
			: m_Width(width), m_Height(height) {}

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		std::string ToString() const override { return fmt::format("{}, Size: [{}, {}]", GetName(), m_Width, m_Height); }

	private:
		uint32_t m_Width;
		uint32_t m_Height;
	};

	class WindowMaximizedEvent : public EventBase<EventType::WindowMaximized, EventCategory::Window>
	{
	public:
		WindowMaximizedEvent(bool maximized)
			: m_Maximized(maximized) {}

		bool GetMaximized() const { return m_Maximized; }
		std::string ToString() const override { return fmt::format("{}, Maximized: {}", GetName(), m_Maximized); }

	private:
		bool m_Maximized;
	};

	class WindowMinimizedEvent : public EventBase<EventType::WindowMinimized, EventCategory::Window>
	{
	public:
		WindowMinimizedEvent(bool minimized)
			: m_Minimized(minimized) {}

		bool GetMinimized() const { return m_Minimized; }
		std::string ToString() const override { return fmt::format("{}, Minimized: {}", GetName(), m_Minimized); }

	private:
		bool m_Minimized;
	};

	class WindowMoveEvent : public EventBase<EventType::WindowMove, EventCategory::Window>
	{
	public:
		WindowMoveEvent(int x, int y)
			: m_X(x), m_Y(y) {}

		int GetX() const { return m_X; }
		int GetY() const { return m_Y; }

		std::string ToString() const override { return fmt::format("{}, Pos: [{}, {}]", GetName(), m_X, m_Y); }

	private:
		int m_X, m_Y;
	};

	class WindowFocusEvent : public EventBase<EventType::WindowFocus, EventCategory::Window>
	{
	};

	class WindowLostFocusEvent : public EventBase<EventType::WindowLostFocus, EventCategory::Window>
	{
	};

	class WindowDropEvent : public EventBase<EventType::WindowDrop, EventCategory::Window>
	{
	public:
		WindowDropEvent(const std::vector<std::filesystem::path>& paths)
			: m_Paths(paths) {}

		WindowDropEvent(std::vector<std::filesystem::path>&& paths)
			: m_Paths(std::move(paths)) {}
		
		const std::vector<std::filesystem::path>& GetPaths() const { return m_Paths; }

		std::string ToString() const override { return fmt::format("{}\n\t- {}", GetName(), fmt::join(m_Paths, "\n\t- ")); }

	private:
		std::vector<std::filesystem::path> m_Paths;
	};

}