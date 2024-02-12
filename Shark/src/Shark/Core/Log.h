#pragma once

#include "Shark/Core/ConsoleSink.h"
#include "Shark/Utils/fmtUtils.h"

#include <spdlog/spdlog.h>

namespace Shark {

	struct Tag
	{
		static constexpr std::string_view None = "";
		static constexpr std::string_view Core = "Core";
		static constexpr std::string_view Serialization = "Serialization";
		static constexpr std::string_view Renderer = "Renderer";
	};

	enum class LogLevel : uint16_t
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

	class Log
	{
	public:
		struct TagSettings
		{
			LogLevel Level = LogLevel::Trace;
			bool Enabled = true;
		};
	public:
		static void Initialize();
		static void Shutdown();

		static void SetConsoleSinkCallback(ConsoleSinkPushMessageCallback callback);

		static std::shared_ptr<spdlog::logger> GetLogger(LoggerType loggerType);
		static std::shared_ptr<spdlog::logger> GetCoreLogger() { return s_Data->CoreLogger; }
		static std::shared_ptr<spdlog::logger> GetClientLogger() { return s_Data->ClientLogger; }
		static std::shared_ptr<spdlog::logger> GetConsoleLogger() { return s_Data->ConsoleLogger; }

		static bool HasTag(std::string_view tag) { return s_Data->EnabledTags.contains(tag); }
		static std::map<std::string_view, TagSettings>& GetTags() { return s_Data->EnabledTags; }

		template<typename... TArgs>
		static void LogMessage(LoggerType loggerType, LogLevel level, std::string_view tag, fmt::format_string<TArgs...> fmt, TArgs&&... args);

		template<typename TFormat, typename... TArgs>
		static void LogMessage(LoggerType loggerType, LogLevel leve, std::string_view tag, const TFormat& fmt, TArgs&&... args);

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

	constexpr std::string_view ToStringView(LogLevel level)
	{
		switch (level)
		{
			case LogLevel::Trace: return "Trace";
			case LogLevel::Debug: return "Debug";
			case LogLevel::Info: return "Info";
			case LogLevel::Warn: return "Warn";
			case LogLevel::Error: return "Error";
			case LogLevel::Critical: return "Critical";
			case LogLevel::Off: return "Off";
		}
		//SK_CORE_ASSERT(false, "Unkown LogLevelType");
		return "Unkown";
	}

}

// Core Logger
#define SK_LOG_IF(arg, logger, level, tag, ...) if ((arg)) { ::Shark::Log::LogMessage(logger, level, tag, __VA_ARGS__); }

#define SK_CORE_TRACE_TAG(tag, ...)         ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Trace, tag, __VA_ARGS__)
#define SK_CORE_DEBUG_TAG(tag, ...)         ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Debug, tag, __VA_ARGS__)
#define SK_CORE_INFO_TAG(tag, ...)          ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Info, tag, __VA_ARGS__)
#define SK_CORE_WARN_TAG(tag, ...)          ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Warn, tag, __VA_ARGS__)
#define SK_CORE_ERROR_TAG(tag, ...)         ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Error, tag, __VA_ARGS__)
#define SK_CORE_CRITICAL_TAG(tag, ...)      ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Critical, tag, __VA_ARGS__)

#define SK_CORE_TRACE(...)                  ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Trace, "", __VA_ARGS__)
#define SK_CORE_DEBUG(...)                  ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Debug, "", __VA_ARGS__)
#define SK_CORE_INFO(...)                   ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Info, "", __VA_ARGS__)
#define SK_CORE_WARN(...)                   ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Warn, "", __VA_ARGS__)
#define SK_CORE_ERROR(...)                  ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Error, "", __VA_ARGS__)
#define SK_CORE_CRITICAL(...)               ::Shark::Log::LogMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Critical, "", __VA_ARGS__)

// Client Log::Lo
#define SK_TRACE_TAG(tag, ...)              ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Trace, tag, __VA_ARGS__)
#define SK_INFO_TAG(tag, ...)               ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Info, tag, __VA_ARGS__)
#define SK_WARN_TAG(tag, ...)               ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Warn, tag, __VA_ARGS__)
#define SK_ERROR_TAG(tag, ...)              ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Error, tag, __VA_ARGS__)
#define SK_CRITICAL_TAG(tag, ...)           ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Critical, tag, __VA_ARGS__)

#define SK_TRACE(...)                       ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Trace, "", __VA_ARGS__)
#define SK_INFO(...)                        ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Info, "", __VA_ARGS__)
#define SK_WARN(...)                        ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Warn, "", __VA_ARGS__)
#define SK_ERROR(...)                       ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Error, "", __VA_ARGS__)
#define SK_CRITICAL(...)                    ::Shark::Log::LogMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Critical, "", __VA_ARGS__)

// Console Logger
#define SK_CONSOLE_TRACE(...)               ::Shark::Log::LogMessage(::Shark::LoggerType::Console, ::Shark::LogLevel::Trace, "", __VA_ARGS__)
#define SK_CONSOLE_DEBUG(...)               ::Shark::Log::LogMessage(::Shark::LoggerType::Console, ::Shark::LogLevel::Debug, "", __VA_ARGS__)
#define SK_CONSOLE_INFO(...)                ::Shark::Log::LogMessage(::Shark::LoggerType::Console, ::Shark::LogLevel::Info, "", __VA_ARGS__)
#define SK_CONSOLE_WARN(...)                ::Shark::Log::LogMessage(::Shark::LoggerType::Console, ::Shark::LogLevel::Warn, "", __VA_ARGS__)
#define SK_CONSOLE_ERROR(...)               ::Shark::Log::LogMessage(::Shark::LoggerType::Console, ::Shark::LogLevel::Error, "", __VA_ARGS__)
#define SK_CONSOLE_CRITICAL(...)            ::Shark::Log::LogMessage(::Shark::LoggerType::Console, ::Shark::LogLevel::Critical, "", __VA_ARGS__)

namespace Shark {

	template<typename... TArgs>
	void Log::LogMessage(LoggerType loggerType, LogLevel level, std::string_view tag, fmt::format_string<TArgs...> fmt, TArgs&&... args)
	{
		auto& setting = s_Data->EnabledTags[tag];
		if (setting.Enabled && level >= setting.Level)
		{
			auto logger = GetLogger(loggerType);
			auto format = fmt::runtime(tag.empty() ? "{0}{1}" : "[{0}] {1}");
			std::string msg = fmt::format(fmt, std::forward<TArgs>(args)...);

			switch (level)
			{
				case LogLevel::Trace:
					logger->trace(format, tag, msg);
					break;
				case LogLevel::Debug:
					logger->debug(format, tag, msg);
					break;
				case LogLevel::Info:
					logger->info(format, tag, msg);
					break;
				case LogLevel::Warn:
					logger->warn(format, tag, msg);
					break;
				case LogLevel::Error:
					logger->error(format, tag, msg);
					break;
				case LogLevel::Critical:
					logger->critical(format, tag, msg);
					break;
			}
		}
	}

	template<typename TFormat, typename... TArgs>
	void Log::LogMessage(LoggerType loggerType, LogLevel level, std::string_view tag, const TFormat& fmt, TArgs&&... args)
	{
		auto& setting = s_Data->EnabledTags[tag];
		if (setting.Enabled && level >= setting.Level)
		{
			auto logger = GetLogger(loggerType);
			auto format = fmt::runtime(tag.empty() ? "{0}{1}" : "[{0}] {1}");
			std::string msg = fmt::format(fmt::runtime(fmt), std::forward<TArgs>(args)...);

			switch (level)
			{
				case LogLevel::Trace:
					logger->trace(format, tag, msg);
					break;
				case LogLevel::Debug:
					logger->debug(format, tag, msg);
					break;
				case LogLevel::Info:
					logger->info(format, tag, msg);
					break;
				case LogLevel::Warn:
					logger->warn(format, tag, msg);
					break;
				case LogLevel::Error:
					logger->error(format, tag, msg);
					break;
				case LogLevel::Critical:
					logger->critical(format, tag, msg);
					break;
			}
		}
	}

	template<typename... TArgs>
	inline void Log::PrintAssertMessage(LoggerType loggerType, std::string_view prefix, TArgs&&... args)
	{
		auto formatMessage = [] <typename TFirst, typename... TArgs> (TFirst format, TArgs&&... args)
		{
			return fmt::format(fmt::runtime(format), std::forward<TArgs>(args)...);
		};

		auto logger = GetLogger(loggerType);
		logger->error("{0}: {1}", prefix, formatMessage(std::forward<TArgs>(args)...));
	}
	
	template<>
	inline void Log::PrintAssertMessage(LoggerType loggerType, std::string_view prefix)
	{
		auto logger = GetLogger(loggerType);
		logger->error("{0}", prefix);
	}

}
