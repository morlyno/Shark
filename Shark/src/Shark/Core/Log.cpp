#include "skpch.h"
#include "Log.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "Shark/Editor/EditorConsole/EditorConsoleSink.h"

#include <array>

namespace Shark {

	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;
	std::shared_ptr<spdlog::logger> Log::s_ConsoleLogger;


	void Log::Init()
	{
		std::vector<spdlog::sink_ptr> sharkSinks =
		{
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/Core.log", true)
		};
		
		std::vector<spdlog::sink_ptr> clientSinks =
		{
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/App.log", true),
			std::make_shared<EditorConsoleSink>(0)
		};

		std::vector<spdlog::sink_ptr> consoleSinks =
		{
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/App.log", true),
			std::make_shared<EditorConsoleSink>(0)
		};

		sharkSinks[0]->set_pattern("%^[%T] %n: %v%$");
		sharkSinks[1]->set_pattern("[%T] [%l] %n: %v");
		clientSinks[0]->set_pattern("%^[%T] %n: %v%$");
		clientSinks[1]->set_pattern("[%T] [%l] %n: %v");
		clientSinks[2]->set_pattern("%^[%T] %n: %v%$");
		consoleSinks[0]->set_pattern("[%T] [%l] %n: %v");
		consoleSinks[1]->set_pattern("%^[%T] %n: %v%$");

		s_CoreLogger = std::make_shared<spdlog::logger>("SHARK", sharkSinks.begin(), sharkSinks.end());
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = std::make_shared<spdlog::logger>("APP", clientSinks.begin(), clientSinks.end());
		s_ClientLogger->set_level(spdlog::level::trace);

		s_ConsoleLogger = std::make_shared<spdlog::logger>("APP", consoleSinks.begin(), consoleSinks.end());
		s_ConsoleLogger->set_level(spdlog::level::trace);
	}

	void Log::Shutdown()
	{
		s_CoreLogger.reset();
		s_ClientLogger.reset();
		s_ConsoleLogger.reset();
	}

}