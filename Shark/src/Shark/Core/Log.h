#pragma once

#include "Shark/Utils/fmtUtils.h"

#include <spdlog/spdlog.h>

namespace Shark {

	class Log
	{
	public:
		enum class Level : uint8_t
		{
			Trace,
			Info,
			Warn,
			Error,
			Critical,
			Debug
		};

	public:
		static void Init();
		static void Shutdown();

		static std::shared_ptr<spdlog::logger> GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger> GetClientLogger() { return s_ClientLogger; }
		static std::shared_ptr<spdlog::logger> GetConsoleLogger() { return s_ConsoleLogger; }

		template<typename... Args>
		static void LogMessage(std::shared_ptr<spdlog::logger> logger, Level level, Args&&... args);

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
		static std::shared_ptr<spdlog::logger> s_ConsoleLogger;
	};

	template<typename... Args>
	void Log::LogMessage(std::shared_ptr<spdlog::logger> logger, Level level, Args&&... args)
	{
		switch (level)
		{
			case Level::Trace:
				logger->trace(std::forward<Args>(args)...);
				break;
			case Level::Info:
				logger->info(std::forward<Args>(args)...);
				break;
			case Level::Warn:
				logger->warn(std::forward<Args>(args)...);
				break;
			case Level::Error:
				logger->error(std::forward<Args>(args)...);
				break;
			case Level::Critical:
				logger->critical(std::forward<Args>(args)...);
				break;
			case Level::Debug:
				logger->debug(std::forward<Args>(args)...);
				break;
		}
	}

}

// Core Logger
#define SK_CORE_LOG(level, ...)        ::Shark::Log::LogMessage(::Shark::Log::GetCoreLogger(), level, __VA_ARGS__)
#define SK_CORE_TRACE(...)             ::Shark::Log::GetCoreLogger()->log(spdlog::level::trace, __VA_ARGS__)
#define SK_CORE_INFO(...)              ::Shark::Log::GetCoreLogger()->log(spdlog::level::info, __VA_ARGS__)
#define SK_CORE_WARN(...)              ::Shark::Log::GetCoreLogger()->log(spdlog::level::warn, __VA_ARGS__)
#define SK_CORE_ERROR(...)             ::Shark::Log::GetCoreLogger()->log(spdlog::level::err, __VA_ARGS__)
#define SK_CORE_CRITICAL(...)          ::Shark::Log::GetCoreLogger()->log(spdlog::level::critical, __VA_ARGS__)

// Client Logger
#define SK_LOG(level, ...)             ::Shark::Log::LogMessage(::Shark::Log::GetClientLogger(), level, __VA_ARGS__)
#define SK_TRACE(...)                  ::Shark::Log::GetClientLogger()->log(spdlog::level::trace, __VA_ARGS__)
#define SK_INFO(...)                   ::Shark::Log::GetClientLogger()->log(spdlog::level::info, __VA_ARGS__)
#define SK_WARN(...)                   ::Shark::Log::GetClientLogger()->log(spdlog::level::warn, __VA_ARGS__)
#define SK_ERROR(...)                  ::Shark::Log::GetClientLogger()->log(spdlog::level::err, __VA_ARGS__)
#define SK_CRITICAL(...)               ::Shark::Log::GetClientLogger()->log(spdlog::level::critical, __VA_ARGS__)

// Console Logger
#define SK_CONSOLE_LOG(level, ...)     ::Shark::Log::LogMessage(::Shark::Log::GetConsoleLogger(), level, __VA_ARGS__)
#define SK_CONSOLE_TRACE(...)          ::Shark::Log::GetConsoleLogger()->log(spdlog::level::trace, __VA_ARGS__)
#define SK_CONSOLE_INFO(...)           ::Shark::Log::GetConsoleLogger()->log(spdlog::level::info, __VA_ARGS__)
#define SK_CONSOLE_WARN(...)           ::Shark::Log::GetConsoleLogger()->log(spdlog::level::warn, __VA_ARGS__)
#define SK_CONSOLE_ERROR(...)          ::Shark::Log::GetConsoleLogger()->log(spdlog::level::err, __VA_ARGS__)
#define SK_CONSOLE_CRITICAL(...)       ::Shark::Log::GetConsoleLogger()->log(spdlog::level::critical, __VA_ARGS__)
