#include "skpch.h"
#include "Log.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "Shark/Editor/EditorConsole/EditorConsoleSink.h"

#include <array>

namespace Shark {

	Log::LogData* Log::s_Data;

	std::shared_ptr<spdlog::logger> Log::GetLogger(Log::Logger loggerType)
	{
		if (loggerType == Log::Logger::Core)
			return s_Data->CoreLogger;
		else if (loggerType == Log::Logger::Client)
			return s_Data->ClientLogger;
		else if (loggerType == Log::Logger::Console)
			return s_Data->ConsoleLogger;
		SK_CORE_ASSERT(false, "Unkown LoggerType");
		return nullptr;
	}


	void Log::Initialize()
	{
		s_Data = new LogData();

		std::vector<spdlog::sink_ptr> sharkSinks =
		{
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/Core.log", true)
		};

		std::vector<spdlog::sink_ptr> clientSinks =
		{
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/App.log", true),
			std::make_shared<EditorConsoleSink>()
		};

		std::vector<spdlog::sink_ptr> consoleSinks =
		{
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/App.log", true),
			std::make_shared<EditorConsoleSink>()
		};

		sharkSinks[0]->set_pattern("%^[%T] %n: %v%$");
		sharkSinks[1]->set_pattern("[%c] [%l] %n: %v");
		clientSinks[0]->set_pattern("%^[%T] %n: %v%$");
		clientSinks[1]->set_pattern("[%c] [%l] %n: %v");
		consoleSinks[0]->set_pattern("[%c] [%l] %n: %v");

		s_Data->CoreLogger = std::make_shared<spdlog::logger>("SHARK", sharkSinks.begin(), sharkSinks.end());
		s_Data->CoreLogger->set_level(spdlog::level::trace);

		s_Data->ClientLogger = std::make_shared<spdlog::logger>("APP", clientSinks.begin(), clientSinks.end());
		s_Data->ClientLogger->set_level(spdlog::level::trace);

		s_Data->ConsoleLogger = std::make_shared<spdlog::logger>("APP", consoleSinks.begin(), consoleSinks.end());
		s_Data->ConsoleLogger->set_level(spdlog::level::trace);
	}

	void Log::Shutdown()
	{
		delete s_Data;
		s_Data = nullptr;
	}

}