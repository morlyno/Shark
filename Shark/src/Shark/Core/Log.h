#pragma once

#include "Shark/Utils/fmtUtils.h"

#include <spdlog/spdlog.h>

namespace Shark {

	class Log
	{
	public:
		enum class Level : uint16_t
		{
			Trace = 0,
			Debug = 1,
			Info = 2,
			Warn = 3,
			Error = 4,
			Critical = 5,
			Off = 6
		};

		enum class Logger : uint16_t
		{
			None = 0,
			Core = 1, Client = 2, Console = 3
		};

		struct TagSettings
		{
			Log::Level Level = Log::Level::Trace;
			bool Enabled = true;
		};
	public:
		static void Initialize();
		static void Shutdown();

		static std::shared_ptr<spdlog::logger> GetLogger(Log::Logger loggerType);
		static std::shared_ptr<spdlog::logger> GetCoreLogger() { return GetCoreLogger(); }
		static std::shared_ptr<spdlog::logger> GetClientLogger() { return GetClientLogger(); }
		static std::shared_ptr<spdlog::logger> GetConsoleLogger() { return GetConsoleLogger(); }

		static bool HasTag(std::string_view tag) { return s_Data->EnabledTags.find(tag) != s_Data->EnabledTags.end(); }
		static std::map<std::string_view, TagSettings>& EnabledTags() { return s_Data->EnabledTags; }

		template<typename... TArgs>
		static void LogMessage(Log::Logger loggerType, Log::Level level, std::string_view tag, TArgs&&... args);

		template<typename... TArgs>
		static void PrintAssertMessage(Log::Logger loggerType, std::string_view prefix, TArgs&&... args);

	private:
		struct LogData
		{
			std::shared_ptr<spdlog::logger> CoreLogger;
			std::shared_ptr<spdlog::logger> ClientLogger;
			std::shared_ptr<spdlog::logger> ConsoleLogger;

			std::map<std::string_view, TagSettings> EnabledTags;
		};
		static LogData* s_Data;

	};

	constexpr std::string_view ToStringView(Log::Level level)
	{
		switch (level)
		{
			case Log::Level::Trace: return "Trace";
			case Log::Level::Debug: return "Debug";
			case Log::Level::Info: return "Info";
			case Log::Level::Warn: return "Warn";
			case Log::Level::Error: return "Error";
			case Log::Level::Critical: return "Critical";
			case Log::Level::Off: return "Off";
		}
		//SK_CORE_ASSERT(false, "Unkown LogLevelType");
		return "Unkown";
	}

}

// Core Logger
#define SK_LOG_IF(arg, logger, level, tag, ...) if ((arg)) { ::Shark::Log::LogMessage(logger, level, tag, __VA_ARGS__); }

#define SK_CORE_TRACE_TAG(tag, ...)         ::Shark::Log::LogMessage(::Shark::Log::Logger::Core, ::Shark::Log::Level::Trace, tag, __VA_ARGS__)
#define SK_CORE_DEBUG_TAG(tag, ...)         ::Shark::Log::LogMessage(::Shark::Log::Logger::Core, ::Shark::Log::Level::Debug, tag, __VA_ARGS__)
#define SK_CORE_INFO_TAG(tag, ...)          ::Shark::Log::LogMessage(::Shark::Log::Logger::Core, ::Shark::Log::Level::Info, tag, __VA_ARGS__)
#define SK_CORE_WARN_TAG(tag, ...)          ::Shark::Log::LogMessage(::Shark::Log::Logger::Core, ::Shark::Log::Level::Warn, tag, __VA_ARGS__)
#define SK_CORE_ERROR_TAG(tag, ...)         ::Shark::Log::LogMessage(::Shark::Log::Logger::Core, ::Shark::Log::Level::Error, tag, __VA_ARGS__)
#define SK_CORE_CRITICAL_TAG(tag, ...)      ::Shark::Log::LogMessage(::Shark::Log::Logger::Core, ::Shark::Log::Level::Critical, tag, __VA_ARGS__)

#define SK_CORE_TRACE(...)                  ::Shark::Log::LogMessage(::Shark::Log::Logger::Core, ::Shark::Log::Level::Trace, "", __VA_ARGS__)
#define SK_CORE_DEBUG(...)                  ::Shark::Log::LogMessage(::Shark::Log::Logger::Core, ::Shark::Log::Level::Debug, "", __VA_ARGS__)
#define SK_CORE_INFO(...)                   ::Shark::Log::LogMessage(::Shark::Log::Logger::Core, ::Shark::Log::Level::Info, "", __VA_ARGS__)
#define SK_CORE_WARN(...)                   ::Shark::Log::LogMessage(::Shark::Log::Logger::Core, ::Shark::Log::Level::Warn, "", __VA_ARGS__)
#define SK_CORE_ERROR(...)                  ::Shark::Log::LogMessage(::Shark::Log::Logger::Core, ::Shark::Log::Level::Error, "", __VA_ARGS__)
#define SK_CORE_CRITICAL(...)               ::Shark::Log::LogMessage(::Shark::Log::Logger::Core, ::Shark::Log::Level::Critical, "", __VA_ARGS__)

// Client Log::Lo
#define SK_TRACE_TAG(tag, ...)              ::Shark::Log::LogMessage(::Shark::Log::Logger::Client, ::Shark::Log::Level::Trace, tag, __VA_ARGS__)
#define SK_INFO_TAG(tag, ...)               ::Shark::Log::LogMessage(::Shark::Log::Logger::Client, ::Shark::Log::Level::Info, tag, __VA_ARGS__)
#define SK_WARN_TAG(tag, ...)               ::Shark::Log::LogMessage(::Shark::Log::Logger::Client, ::Shark::Log::Level::Warn, tag, __VA_ARGS__)
#define SK_ERROR_TAG(tag, ...)              ::Shark::Log::LogMessage(::Shark::Log::Logger::Client, ::Shark::Log::Level::Error, tag, __VA_ARGS__)
#define SK_CRITICAL_TAG(tag, ...)           ::Shark::Log::LogMessage(::Shark::Log::Logger::Client, ::Shark::Log::Level::Critical, tag, __VA_ARGS__)

#define SK_TRACE(...)                       ::Shark::Log::LogMessage(::Shark::Log::Logger::Client, ::Shark::Log::Level::Trace, "", __VA_ARGS__)
#define SK_INFO(...)                        ::Shark::Log::LogMessage(::Shark::Log::Logger::Client, ::Shark::Log::Level::Info, "", __VA_ARGS__)
#define SK_WARN(...)                        ::Shark::Log::LogMessage(::Shark::Log::Logger::Client, ::Shark::Log::Level::Warn, "", __VA_ARGS__)
#define SK_ERROR(...)                       ::Shark::Log::LogMessage(::Shark::Log::Logger::Client, ::Shark::Log::Level::Error, "", __VA_ARGS__)
#define SK_CRITICAL(...)                    ::Shark::Log::LogMessage(::Shark::Log::Logger::Client, ::Shark::Log::Level::Critical, "", __VA_ARGS__)

// Console Logger
#define SK_CONSOLE_TRACE(...)               ::Shark::Log::LogMessage(::Shark::Log::Logger::Console, ::Shark::Log::Level::Trace, "", __VA_ARGS__)
#define SK_CONSOLE_DEBUG(...)               ::Shark::Log::LogMessage(::Shark::Log::Logger::Console, ::Shark::Log::Level::Debug, "", __VA_ARGS__)
#define SK_CONSOLE_INFO(...)                ::Shark::Log::LogMessage(::Shark::Log::Logger::Console, ::Shark::Log::Level::Info, "", __VA_ARGS__)
#define SK_CONSOLE_WARN(...)                ::Shark::Log::LogMessage(::Shark::Log::Logger::Console, ::Shark::Log::Level::Warn, "", __VA_ARGS__)
#define SK_CONSOLE_ERROR(...)               ::Shark::Log::LogMessage(::Shark::Log::Logger::Console, ::Shark::Log::Level::Error, "", __VA_ARGS__)
#define SK_CONSOLE_CRITICAL(...)            ::Shark::Log::LogMessage(::Shark::Log::Logger::Console, ::Shark::Log::Level::Critical, "", __VA_ARGS__)

namespace Shark {

	template<typename... TArgs>
	void Log::LogMessage(Log::Logger loggerType, Log::Level level, std::string_view tag, TArgs&&... args)
	{
		auto& setting = s_Data->EnabledTags[tag];
		if (setting.Enabled && level >= setting.Level)
		{
			auto logger = GetLogger(loggerType);
			std::string format = tag.empty() ? "{0}{1}" : "[{0}] {1}";

			switch (level)
			{
				case Log::Level::Trace:
					logger->trace(format, tag, fmt::format(std::forward<TArgs>(args)...));
					break;
				case Log::Level::Debug:
					logger->debug(format, tag, fmt::format(std::forward<TArgs>(args)...));
					break;
				case Log::Level::Info:
					logger->info(format, tag, fmt::format(std::forward<TArgs>(args)...));
					break;
				case Log::Level::Warn:
					logger->warn(format, tag, fmt::format(std::forward<TArgs>(args)...));
					break;
				case Log::Level::Error:
					logger->error(format, tag, fmt::format(std::forward<TArgs>(args)...));
					break;
				case Log::Level::Critical:
					logger->critical(format, tag, fmt::format(std::forward<TArgs>(args)...));
					break;
			}
		}
	}

	template<typename... TArgs>
	inline void Log::PrintAssertMessage(Log::Logger loggerType, std::string_view prefix, TArgs&&... args)
	{
		auto logger = GetLogger(loggerType);
		logger->error("{0}: {1}", prefix, fmt::format(std::forward<TArgs>(args)...));
	}
	
	template<>
	inline void Log::PrintAssertMessage(Log::Logger loggerType, std::string_view prefix)
	{
		auto logger = GetLogger(loggerType);
		logger->error("{0}", prefix);
	}

}
