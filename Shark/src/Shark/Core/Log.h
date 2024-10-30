#pragma once

#include "Shark/Utils/fmtUtils.h"

#include <spdlog/spdlog.h>

namespace Shark {

	struct ConsoleSinkMessage;

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

	struct TagSettings
	{
		bool Enabled = true;
		LogLevel Level = LogLevel::Trace;
	};

	class Log
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void SetConsoleSinkCallback(std::function<void(ConsoleSinkMessage&&)> callback);
		static spdlog::sink_ptr GetConsoleSink();

		static std::shared_ptr<spdlog::logger> GetLogger(LoggerType loggerType);
		static std::shared_ptr<spdlog::logger> GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger> GetClientLogger() { return s_ClientLogger; }
		static std::shared_ptr<spdlog::logger> GetConsoleLogger() { return s_ConsoleLogger; }

		static bool HasTag(const std::string& tag) { return s_EnabledTags.contains(tag); }
		static std::map<std::string, TagSettings>& EnabledTags() { return s_EnabledTags; }

		template<typename... TArgs>
		static void PrintMessage(LoggerType loggerType, LogLevel level, fmt::format_string<TArgs...> format, TArgs&&... args);
		static void PrintMessage(LoggerType loggerType, LogLevel level, std::string_view message);

		template<typename... TArgs>
		static void PrintMessageTag(LoggerType loggerType, LogLevel level, std::string_view tag, fmt::format_string<TArgs...> format, TArgs&&... args);
		static void PrintMessageTag(LoggerType loggerType, LogLevel level, std::string_view tag, std::string_view message);

		template<typename... TArgs>
		static void PrintAssertMessage(LoggerType loggerType, std::string_view prefix, fmt::format_string<TArgs...> format, TArgs&&... args);
		static void PrintAssertMessage(LoggerType loggerType, std::string_view prefix);

	private:
		static inline std::shared_ptr<spdlog::logger> s_CoreLogger;
		static inline std::shared_ptr<spdlog::logger> s_ClientLogger;
		static inline std::shared_ptr<spdlog::logger> s_ConsoleLogger;

		static inline std::map<std::string, TagSettings> s_EnabledTags;
	};

}

#define SK_LOG_IF(arg, logger, level, tag, ...) if ((arg)) { ::Shark::Log::PrintMessageTag(logger, level, tag, __VA_ARGS__); }

// Core Logger
#define SK_CORE_TRACE_TAG(tag, ...)         ::Shark::Log::PrintMessageTag(::Shark::LoggerType::Core, ::Shark::LogLevel::Trace, tag, __VA_ARGS__)
#define SK_CORE_DEBUG_TAG(tag, ...)         ::Shark::Log::PrintMessageTag(::Shark::LoggerType::Core, ::Shark::LogLevel::Debug, tag, __VA_ARGS__)
#define SK_CORE_INFO_TAG(tag, ...)          ::Shark::Log::PrintMessageTag(::Shark::LoggerType::Core, ::Shark::LogLevel::Info, tag, __VA_ARGS__)
#define SK_CORE_WARN_TAG(tag, ...)          ::Shark::Log::PrintMessageTag(::Shark::LoggerType::Core, ::Shark::LogLevel::Warn, tag, __VA_ARGS__)
#define SK_CORE_ERROR_TAG(tag, ...)         ::Shark::Log::PrintMessageTag(::Shark::LoggerType::Core, ::Shark::LogLevel::Error, tag, __VA_ARGS__)
#define SK_CORE_CRITICAL_TAG(tag, ...)      ::Shark::Log::PrintMessageTag(::Shark::LoggerType::Core, ::Shark::LogLevel::Critical, tag, __VA_ARGS__)

#define SK_CORE_TRACE(...)                  ::Shark::Log::PrintMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Trace, __VA_ARGS__)
#define SK_CORE_DEBUG(...)                  ::Shark::Log::PrintMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Debug, __VA_ARGS__)
#define SK_CORE_INFO(...)                   ::Shark::Log::PrintMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Info, __VA_ARGS__)
#define SK_CORE_WARN(...)                   ::Shark::Log::PrintMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Warn, __VA_ARGS__)
#define SK_CORE_ERROR(...)                  ::Shark::Log::PrintMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Error, __VA_ARGS__)
#define SK_CORE_CRITICAL(...)               ::Shark::Log::PrintMessage(::Shark::LoggerType::Core, ::Shark::LogLevel::Critical, __VA_ARGS__)

// Client Logger
#define SK_TRACE_TAG(tag, ...)              ::Shark::Log::PrintMessageTag(::Shark::LoggerType::Client, ::Shark::LogLevel::Trace, tag, __VA_ARGS__)
#define SK_INFO_TAG(tag, ...)               ::Shark::Log::PrintMessageTag(::Shark::LoggerType::Client, ::Shark::LogLevel::Info, tag, __VA_ARGS__)
#define SK_WARN_TAG(tag, ...)               ::Shark::Log::PrintMessageTag(::Shark::LoggerType::Client, ::Shark::LogLevel::Warn, tag, __VA_ARGS__)
#define SK_ERROR_TAG(tag, ...)              ::Shark::Log::PrintMessageTag(::Shark::LoggerType::Client, ::Shark::LogLevel::Error, tag, __VA_ARGS__)
#define SK_CRITICAL_TAG(tag, ...)           ::Shark::Log::PrintMessageTag(::Shark::LoggerType::Client, ::Shark::LogLevel::Critical, tag, __VA_ARGS__)

#define SK_TRACE(...)                       ::Shark::Log::PrintMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Trace, __VA_ARGS__)
#define SK_INFO(...)                        ::Shark::Log::PrintMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Info, __VA_ARGS__)
#define SK_WARN(...)                        ::Shark::Log::PrintMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Warn, __VA_ARGS__)
#define SK_ERROR(...)                       ::Shark::Log::PrintMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Error, __VA_ARGS__)
#define SK_CRITICAL(...)                    ::Shark::Log::PrintMessage(::Shark::LoggerType::Client, ::Shark::LogLevel::Critical, __VA_ARGS__)

// Console Logger
#define SK_CONSOLE_TRACE(...)               ::Shark::Log::PrintMessage(::Shark::LoggerType::Console, ::Shark::LogLevel::Trace, __VA_ARGS__)
#define SK_CONSOLE_DEBUG(...)               ::Shark::Log::PrintMessage(::Shark::LoggerType::Console, ::Shark::LogLevel::Debug, __VA_ARGS__)
#define SK_CONSOLE_INFO(...)                ::Shark::Log::PrintMessage(::Shark::LoggerType::Console, ::Shark::LogLevel::Info, __VA_ARGS__)
#define SK_CONSOLE_WARN(...)                ::Shark::Log::PrintMessage(::Shark::LoggerType::Console, ::Shark::LogLevel::Warn, __VA_ARGS__)
#define SK_CONSOLE_ERROR(...)               ::Shark::Log::PrintMessage(::Shark::LoggerType::Console, ::Shark::LogLevel::Error, __VA_ARGS__)
#define SK_CONSOLE_CRITICAL(...)            ::Shark::Log::PrintMessage(::Shark::LoggerType::Console, ::Shark::LogLevel::Critical, __VA_ARGS__)

namespace Shark {

	template<typename... TArgs>
	void Log::PrintMessage(LoggerType loggerType, LogLevel level, fmt::format_string<TArgs...> format, TArgs&&... args)
	{
		auto& settings = s_EnabledTags[""];
		if (settings.Enabled && level >= settings.Level)
		{
			auto logger = GetLogger(loggerType);

			switch (level)
			{
				case LogLevel::Trace: logger->trace(format, std::forward<TArgs>(args)...); break;
				case LogLevel::Debug: logger->debug(format, std::forward<TArgs>(args)...); break;
				case LogLevel::Info: logger->info(format, std::forward<TArgs>(args)...); break;
				case LogLevel::Warn: logger->warn(format, std::forward<TArgs>(args)...); break;
				case LogLevel::Error: logger->error(format, std::forward<TArgs>(args)...); break;
				case LogLevel::Critical: logger->critical(format, std::forward<TArgs>(args)...); break;
			}
		}
	}

	template<typename... TArgs>
	void Log::PrintMessageTag(LoggerType loggerType, LogLevel level, std::string_view tag, fmt::format_string<TArgs...> format, TArgs&&... args)
	{
		auto& settings = s_EnabledTags[std::string(tag)];
		if (settings.Enabled && level >= settings.Level)
		{
			auto logger = GetLogger(loggerType);
			std::string formatted = fmt::format(format, std::forward<TArgs>(args)...);

			switch (level)
			{
				case LogLevel::Trace: logger->trace("[{}] {}", tag, formatted); break;
				case LogLevel::Debug: logger->debug("[{}] {}", tag, formatted); break;
				case LogLevel::Info: logger->info("[{}] {}", tag, formatted); break;
				case LogLevel::Warn: logger->warn("[{}] {}", tag, formatted); break;
				case LogLevel::Error: logger->error("[{}] {}", tag, formatted); break;
				case LogLevel::Critical: logger->critical("[{}] {}", tag, formatted); break;
			}
		}
	}

	template<typename... TArgs>
	inline void Log::PrintAssertMessage(LoggerType loggerType, std::string_view prefix, fmt::format_string<TArgs...> format, TArgs&&... args)
	{
		auto logger = GetLogger(loggerType);
		logger->error("{0}: {1}", prefix, fmt::format(format, std::forward<TArgs>(args)...));
	}
	
	inline void Log::PrintAssertMessage(LoggerType loggerType, std::string_view prefix)
	{
		auto logger = GetLogger(loggerType);
		logger->error("{0}", prefix);
	}

}
