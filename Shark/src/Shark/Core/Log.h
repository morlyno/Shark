#pragma once

#include "Shark/Utils/fmtUtils.h"

#include <spdlog/spdlog.h>

namespace Shark {

	enum class LogLevelType : uint16_t
	{
		Trace = 0,
		Debug = 1,
		Info = 2,
		Warn = 3,
		Error = 4,
		Critical = 5,
		Off = 6
	};

	enum class LoggerType : uint16_t
	{
		None = 0,
		Core = 1, Client = 2, Console = 3
	};

	constexpr std::string_view ToString(LogLevelType level)
	{
		switch (level)
		{
			case LogLevelType::Trace: return "Trace";
			case LogLevelType::Debug: return "Debug";
			case LogLevelType::Info: return "Info";
			case LogLevelType::Warn: return "Warn";
			case LogLevelType::Error: return "Error";
			case LogLevelType::Critical: return "Critical";
			case LogLevelType::Off: return "Off";
		}
		//SK_CORE_ASSERT(false, "Unkown LogLevelType");
		return "Unkown";
	}

	class Log
	{
	public:
		struct TagSettings
		{
			LogLevelType Level = LogLevelType::Trace;
			bool Enabled = true;
		};
	public:
		static void Initialize();
		static void Shutdown();

		static std::shared_ptr<spdlog::logger> GetLogger(LoggerType loggerType);
		static std::shared_ptr<spdlog::logger> GetCoreLogger() { return GetCoreLogger(); }
		static std::shared_ptr<spdlog::logger> GetClientLogger() { return GetClientLogger(); }
		static std::shared_ptr<spdlog::logger> GetConsoleLogger() { return GetConsoleLogger(); }

		static bool HasTag(std::string_view tag) { return s_Data->EnabledTags.find(tag) != s_Data->EnabledTags.end(); }
		static std::map<std::string_view, TagSettings>& EnabledTags() { return s_Data->EnabledTags; }

		template<typename... TArgs>
		static void LogMessage(LoggerType loggerType, LogLevelType level, std::string_view tag, TArgs&&... args);

		template<typename... TArgs>
		static void PrintAssertMessage(LoggerType loggerType, std::string_view prefix, TArgs&&... args);

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

}

// Core Logger
#define SK_CORE_TRACE_TAG(tag, ...)         ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevelType::Trace, tag, __VA_ARGS__)
#define SK_CORE_DEBUG_TAG(tag, ...)         ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevelType::Debug, tag, __VA_ARGS__)
#define SK_CORE_INFO_TAG(tag, ...)          ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevelType::Info, tag, __VA_ARGS__)
#define SK_CORE_WARN_TAG(tag, ...)          ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevelType::Warn, tag, __VA_ARGS__)
#define SK_CORE_ERROR_TAG(tag, ...)         ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevelType::Error, tag, __VA_ARGS__)
#define SK_CORE_CRITICAL_TAG(tag, ...)      ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevelType::Critical, tag, __VA_ARGS__)

#define SK_CORE_TRACE(...)                  ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevelType::Trace, "", __VA_ARGS__)
#define SK_CORE_DEBUG(...)                  ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevelType::Debug, "", __VA_ARGS__)
#define SK_CORE_INFO(...)                   ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevelType::Info, "", __VA_ARGS__)
#define SK_CORE_WARN(...)                   ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevelType::Warn, "", __VA_ARGS__)
#define SK_CORE_ERROR(...)                  ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevelType::Error, "", __VA_ARGS__)
#define SK_CORE_CRITICAL(...)               ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevelType::Critical, "", __VA_ARGS__)

// Client Logger
#define SK_TRACE_TAG(tag, ...)              ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevelType::Trace, tag, __VA_ARGS__)
#define SK_INFO_TAG(tag, ...)               ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevelType::Info, tag, __VA_ARGS__)
#define SK_WARN_TAG(tag, ...)               ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevelType::Warn, tag, __VA_ARGS__)
#define SK_ERROR_TAG(tag, ...)              ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevelType::Error, tag, __VA_ARGS__)
#define SK_CRITICAL_TAG(tag, ...)           ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevelType::Critical, tag, __VA_ARGS__)

#define SK_TRACE(...)                       ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevelType::Trace, "", __VA_ARGS__)
#define SK_INFO(...)                        ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevelType::Info, "", __VA_ARGS__)
#define SK_WARN(...)                        ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevelType::Warn, "", __VA_ARGS__)
#define SK_ERROR(...)                       ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevelType::Error, "", __VA_ARGS__)
#define SK_CRITICAL(...)                    ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevelType::Critical, "", __VA_ARGS__)

// Console Logger
#define SK_CONSOLE_TRACE(...)               ::Shark::Log::LogMessage(::Shark::LoggerType::Console, ::Shark::LogLevelType::Trace, "", __VA_ARGS__)
#define SK_CONSOLE_DEBUG(...)               ::Shark::Log::LogMessage(::Shark::LoggerType::Console, ::Shark::LogLevelType::Debug, "", __VA_ARGS__)
#define SK_CONSOLE_INFO(...)                ::Shark::Log::LogMessage(::Shark::LoggerType::Console, ::Shark::LogLevelType::Info, "", __VA_ARGS__)
#define SK_CONSOLE_WARN(...)                ::Shark::Log::LogMessage(::Shark::LoggerType::Console, ::Shark::LogLevelType::Warn, "", __VA_ARGS__)
#define SK_CONSOLE_ERROR(...)               ::Shark::Log::LogMessage(::Shark::LoggerType::Console, ::Shark::LogLevelType::Error, "", __VA_ARGS__)
#define SK_CONSOLE_CRITICAL(...)            ::Shark::Log::LogMessage(::Shark::LoggerType::Console, ::Shark::LogLevelType::Critical, "", __VA_ARGS__)

namespace Shark {

	template<typename... TArgs>
	void Log::LogMessage(LoggerType loggerType, LogLevelType level, std::string_view tag, TArgs&&... args)
	{
		auto& setting = s_Data->EnabledTags[tag];
		if (setting.Enabled && level >= setting.Level)
		{
			auto logger = GetLogger(loggerType);
			std::string format = tag.empty() ? "{0}{1}" : "[{0}] {1}";

			switch (level)
			{
				case LogLevelType::Trace:
					logger->trace(format, tag, fmt::format(std::forward<TArgs>(args)...));
					break;
				case LogLevelType::Debug:
					logger->debug(format, tag, fmt::format(std::forward<TArgs>(args)...));
					break;
				case LogLevelType::Info:
					logger->info(format, tag, fmt::format(std::forward<TArgs>(args)...));
					break;
				case LogLevelType::Warn:
					logger->warn(format, tag, fmt::format(std::forward<TArgs>(args)...));
					break;
				case LogLevelType::Error:
					logger->error(format, tag, fmt::format(std::forward<TArgs>(args)...));
					break;
				case LogLevelType::Critical:
					logger->critical(format, tag, fmt::format(std::forward<TArgs>(args)...));
					break;
			}
		}
	}

	template<typename... TArgs>
	inline void Log::PrintAssertMessage(LoggerType loggerType, std::string_view prefix, TArgs&&... args)
	{
		auto logger = GetLogger(loggerType);
		logger->error("{0}: {1}", prefix, fmt::format(std::forward<TArgs>(args)...));
	}
	
	template<>
	inline void Log::PrintAssertMessage(LoggerType loggerType, std::string_view prefix)
	{
		auto logger = GetLogger(loggerType);
		logger->error("{0}", prefix);
	}

}
