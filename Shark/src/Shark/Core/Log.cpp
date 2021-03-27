#include "skpch.h"
#include "Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include <stdarg.h>

namespace Shark {

	std::shared_ptr<spdlog::logger> Log::s_Core_Logger;
	std::shared_ptr<spdlog::logger> Log::s_Client_Logger;

	std::shared_ptr<spdlog::logger>(*Log::GetCoreLogger)() = Log::InitPtrCore;
	std::shared_ptr<spdlog::logger>(*Log::GetClientLogger)() = Log::InitPtrClient;

	void Log::Init()
	{
		spdlog::set_pattern("[%H:%M:%S] %n: %v%$");
		s_Core_Logger = spdlog::stdout_color_mt("SHARK");
		s_Core_Logger->set_level(spdlog::level::trace);

		s_Client_Logger = spdlog::stdout_color_mt("APP");
		s_Client_Logger->set_level(spdlog::level::trace);

		GetCoreLogger = GetCoreLoggerImpl;
		GetClientLogger = GetClientLoggerImpl;

	}

}