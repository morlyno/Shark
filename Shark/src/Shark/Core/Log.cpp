#include "skpch.h"
#include "Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <stdarg.h>

namespace Shark {

	std::shared_ptr<spdlog::logger> Log::s_Core_Logger;
	std::shared_ptr<spdlog::logger> Log::s_Client_Logger;

	void Log::Init()
	{
		spdlog::set_pattern("[%T] %n: %v%$");
		spdlog::set_level(spdlog::level::trace);


		spdlog::sinks_init_list core_sinks =
		{
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("Log/Shark.log", true)
		};
		s_Core_Logger = std::make_shared<spdlog::logger>("SHARK", core_sinks);
		spdlog::initialize_logger(s_Core_Logger);
		s_Core_Logger->sinks()[1]->set_pattern("[%T] [%l] %n: %v%$");


		spdlog::sinks_init_list app_sinks =
		{
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("Log/APP.log", true)
		};
		s_Client_Logger = std::make_shared<spdlog::logger>("APP", app_sinks);
		spdlog::initialize_logger(s_Client_Logger);
		s_Client_Logger->sinks()[1]->set_pattern("[%T] [%l] %n: %v%$");

	}

}