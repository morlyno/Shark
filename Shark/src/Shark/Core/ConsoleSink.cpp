#include "skpch.h"
#include "ConsoleSink.h"

namespace Shark {

	namespace utils {

		LogLevel spdlogLevelToLogLevel(spdlog::level::level_enum level)
		{
			switch (level)
			{
				case spdlog::level::trace:     return LogLevel::Trace;
				case spdlog::level::debug:     return LogLevel::Debug;
				case spdlog::level::info:      return LogLevel::Info;
				case spdlog::level::warn:      return LogLevel::Warn;
				case spdlog::level::err:       return LogLevel::Error;
				case spdlog::level::critical:  return LogLevel::Critical;
			}
			SK_CORE_ASSERT(false, "Unkown Log Level");
			return LogLevel::Trace;
		}

	}

	ConsoleSink::ConsoleSink()
		: m_MessageFormatter(std::make_unique<spdlog::pattern_formatter>()), m_TimeFormatter(std::make_unique<spdlog::pattern_formatter>())
	{
		m_MessageFormatter = std::make_unique<spdlog::pattern_formatter>("%v", spdlog::pattern_time_type::local, std::string{});
		m_TimeFormatter = std::make_unique<spdlog::pattern_formatter>("%T", spdlog::pattern_time_type::local, std::string{});
	}

	ConsoleSink::~ConsoleSink()
	{
	}

	void ConsoleSink::log(const spdlog::details::log_msg& msg)
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
		
		if (m_PushMessageCallback)
			m_PushMessageCallback({ utils::spdlogLevelToLogLevel(msg.level), m_CachedTime, fmt::to_string(buffer) });
	}

	void ConsoleSink::flush()
	{
	}

	void ConsoleSink::set_pattern(const std::string& pattern)
	{
		std::lock_guard lock(m_Mutex);
		m_MessageFormatter = std::unique_ptr<spdlog::formatter>(sknew spdlog::pattern_formatter(pattern));
	}

	void ConsoleSink::set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter)
	{
		std::lock_guard lock(m_Mutex);
		m_MessageFormatter = std::move(sink_formatter);
	}

}
