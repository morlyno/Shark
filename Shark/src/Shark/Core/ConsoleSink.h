#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>

namespace Shark {

	enum class LogLevel : uint16_t;

	struct ConsoleSinkMessage
	{
		LogLevel MessageLevel;
		std::string Timepoint;
		std::string Message;
	};
	using ConsoleSinkPushMessageCallback = std::function<void(ConsoleSinkMessage&&)>;

	class ConsoleSink : public spdlog::sinks::sink
	{
	public:
		ConsoleSink();
		~ConsoleSink();

		void log(const spdlog::details::log_msg& msg) override;
		void flush() override;
		void set_pattern(const std::string& pattern) override;
		void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override;

		void SetPushMessageCallback(ConsoleSinkPushMessageCallback  callback) { m_PushMessageCallback = callback; }

	private:
		std::mutex m_Mutex;
		std::unique_ptr<spdlog::formatter> m_MessageFormatter;
		std::unique_ptr<spdlog::formatter> m_TimeFormatter;
		std::string m_CachedTime;
		std::chrono::seconds m_LastFormatTime;

		ConsoleSinkPushMessageCallback m_PushMessageCallback = nullptr;
	};

}