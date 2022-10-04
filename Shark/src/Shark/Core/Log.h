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
		struct TagData
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
		static const std::map<std::string_view, TagData>& GetTags() { return s_Data->Tags; }
		static std::map<std::string_view, TagData>& GetMutableTags() { return s_Data->Tags; }

		static void EnableTag(std::string_view tag, bool enabled) { s_Data->Tags[tag].Enabled = enabled; }
		static void SetTagLevel(std::string_view tag, LogLevelType level) { s_Data->Tags[tag].Level = level; }

		template<typename TFirst, typename... TRest>
		static void LogMessage(LoggerType loggerType, LogLevelType level, std::string_view tag, TFirst&& firstArg, TRest&&... args)
		{
			if (!tag.empty())
			{
				auto& tagData = s_Data->Tags[tag];
				if (!tagData.Enabled || level < tagData.Level)
					return;
			}

			auto logger = GetLogger(loggerType);
			auto format = fmt::format(tag.empty() ? "{1}" : "[{0}] {1}", tag, std::forward<TFirst>(firstArg));

			switch (level)
			{
				case LogLevelType::Trace:
					logger->trace(format, std::forward<TRest>(args)...);
					break;
				case LogLevelType::Debug:
					logger->debug(format, std::forward<TRest>(args)...);
					break;
				case LogLevelType::Info:
					logger->info(format, std::forward<TRest>(args)...);
					break;
				case LogLevelType::Warn:
					logger->warn(format, std::forward<TRest>(args)...);
					break;
				case LogLevelType::Error:
					logger->error(format, std::forward<TRest>(args)...);
					break;
				case LogLevelType::Critical:
					logger->critical(format, std::forward<TRest>(args)...);
					break;
			}
		}

	private:
		struct LogData
		{
			std::shared_ptr<spdlog::logger> CoreLogger;
			std::shared_ptr<spdlog::logger> ClientLogger;
			std::shared_ptr<spdlog::logger> ConsoleLogger;

			std::map<std::string_view, TagData> Tags;
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
