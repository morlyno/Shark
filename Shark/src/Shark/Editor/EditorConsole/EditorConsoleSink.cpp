#include "skpch.h"
#include "EditorConsoleSink.h"

#include "Shark/Editor/EditorConsole/EditorConsolePanel.h"

namespace Shark {

	namespace utils {

		LogLevelType spdlogLevelToLogLevel(spdlog::level::level_enum level)
		{
			switch (level)
			{
				case spdlog::level::trace:     return LogLevelType::Trace;
				case spdlog::level::debug:     return LogLevelType::Debug;
				case spdlog::level::info:      return LogLevelType::Info;
				case spdlog::level::warn:      return LogLevelType::Warn;
				case spdlog::level::err:       return LogLevelType::Error;
				case spdlog::level::critical:  return LogLevelType::Critical;
			}
			SK_CORE_ASSERT(false, "Unkown Log Level");
			return LogLevelType::Trace;
		}

	}

	EditorConsoleSink::EditorConsoleSink()
		: m_MessageFormatter(std::make_unique<spdlog::pattern_formatter>()), m_TimeFormatter(std::make_unique<spdlog::pattern_formatter>())
	{
		m_MessageFormatter = std::make_unique<spdlog::pattern_formatter>("%v", spdlog::pattern_time_type::local, std::string{});
		m_TimeFormatter = std::make_unique<spdlog::pattern_formatter>("%T", spdlog::pattern_time_type::local, std::string{});
	}

	EditorConsoleSink::~EditorConsoleSink()
	{
	}

	void EditorConsoleSink::log(const spdlog::details::log_msg& msg)
	{
		std::lock_guard lock(m_Mutex);

		const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(msg.time.time_since_epoch());
		if (seconds != m_LastFormatTime)
		{
			spdlog::memory_buf_t buffer;
			m_TimeFormatter->format(msg, buffer);
			m_CachedTime = fmt::to_string(buffer);
			m_LastFormatTime = seconds;
		}

		spdlog::memory_buf_t buffer;
		m_MessageFormatter->format(msg, buffer);
		EditorConsolePanel::PushMessage(utils::spdlogLevelToLogLevel(msg.level), m_CachedTime, fmt::to_string(buffer));
	}

	void EditorConsoleSink::flush()
	{

	}

	void EditorConsoleSink::set_pattern(const std::string& pattern)
	{
		std::lock_guard lock(m_Mutex);
		m_MessageFormatter = std::unique_ptr<spdlog::formatter>(new spdlog::pattern_formatter(pattern));
	}

	void EditorConsoleSink::set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter)
	{
		std::lock_guard lock(m_Mutex);
		m_MessageFormatter = std::move(sink_formatter);
	}

}
