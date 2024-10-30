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

	ConsoleSink::ConsoleSink(uint32_t capacity)
		: m_MessageBufferCapacity(capacity)
	{
		set_pattern("%v");
	}

	ConsoleSink::~ConsoleSink()
	{
	}

	void ConsoleSink::sink_it_(const spdlog::details::log_msg& msg)
	{
		spdlog::memory_buf_t formatted;
		formatter_->format(msg, formatted);

		m_MessageBuffer.emplace_back(GetMessageLevel(msg.level), fmt::to_string(formatted), msg.time);

		if (m_MessageBuffer.size() >= m_MessageBufferCapacity)
			flush_();
	}

	void ConsoleSink::flush_()
	{
		for (auto& message : m_MessageBuffer)
			m_PushMessageCallback(std::move(message));

		m_MessageBuffer.clear();
	}

	Shark::LogLevel ConsoleSink::GetMessageLevel(spdlog::level::level_enum level) const
	{
		switch (level)
		{
			case spdlog::level::trace: return LogLevel::Trace;
			case spdlog::level::debug: return LogLevel::Debug;
			case spdlog::level::info: return LogLevel::Info;
			case spdlog::level::warn: return LogLevel::Warn;
			case spdlog::level::err: return LogLevel::Error;
			case spdlog::level::critical: return LogLevel::Critical;
			case spdlog::level::off: return LogLevel::Off;
		}

		SK_CORE_VERIFY(false, "Unkown spdlog level");
		return LogLevel::Off;
	}

}
