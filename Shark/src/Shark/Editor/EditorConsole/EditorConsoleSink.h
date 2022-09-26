#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>

namespace Shark {

	class EditorConsoleSink : public spdlog::sinks::sink
	{
	public:
		EditorConsoleSink();
		~EditorConsoleSink();

		void log(const spdlog::details::log_msg& msg) override;
		void flush() override;
		void set_pattern(const std::string& pattern) override;
		void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) override;

	private:
		std::mutex m_Mutex;
		std::unique_ptr<spdlog::formatter> m_MessageFormatter;
		std::unique_ptr<spdlog::formatter> m_TimeFormatter;

		std::string m_CachedTime;
		std::chrono::seconds m_LastFormatTime;
	};

}
