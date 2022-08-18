#include "skpch.h"
#include "EditorConsoleSink.h"

#include "Shark/Editor/EditorConsole/EditorConsolePanel.h"

namespace Shark {

	namespace utils {

		Log::Level spdlogLevelToLogLevel(spdlog::level::level_enum level)
		{
			switch (level)
			{
				case spdlog::level::trace:     return Log::Level::Trace;
				case spdlog::level::debug:     return Log::Level::Debug;
				case spdlog::level::info:      return Log::Level::Info;
				case spdlog::level::warn:      return Log::Level::Warn;
				case spdlog::level::err:       return Log::Level::Error;
				case spdlog::level::critical:  return Log::Level::Critical;
			}
			SK_CORE_ASSERT(false, "Unkown Log Level");
			return Log::Level::Trace;
		}

	}

	EditorConsoleSink::EditorConsoleSink()
		: m_Formatter(std::make_unique<spdlog::pattern_formatter>())
	{
	}

	EditorConsoleSink::~EditorConsoleSink()
	{
	}

	void EditorConsoleSink::log(const spdlog::details::log_msg& msg)
	{

		std::lock_guard lock(m_Mutex);

		spdlog::memory_buf_t buffer;
		m_Formatter->format(msg, buffer);
		EditorConsolePanel::PushMessage(utils::spdlogLevelToLogLevel(msg.level), fmt::to_string(buffer));
	}

	void EditorConsoleSink::flush()
	{

	}

	void EditorConsoleSink::set_pattern(const std::string& pattern)
	{
		std::lock_guard lock(m_Mutex);
		m_Formatter = std::unique_ptr<spdlog::formatter>(new spdlog::pattern_formatter(pattern));
	}

	void EditorConsoleSink::set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter)
	{
		std::lock_guard lock(m_Mutex);
		m_Formatter = std::move(sink_formatter);
	}

}
