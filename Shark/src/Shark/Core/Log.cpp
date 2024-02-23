#include "skpch.h"
#include "Log.h"

#include "Shark/Core/ConsoleSink.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace Shark {

	Log::LogData* Log::s_Data;
	static std::shared_ptr<ConsoleSink> s_ConsoleSink = nullptr;

	std::shared_ptr<spdlog::logger> Log::GetLogger(LoggerType loggerType)
	{
		if (loggerType == LoggerType::Core)
			return s_Data->CoreLogger;
		else if (loggerType == LoggerType::Client)
			return s_Data->ClientLogger;
		else if (loggerType == LoggerType::Console)
			return s_Data->ConsoleLogger;
		SK_CORE_ASSERT(false, "Unkown LoggerType");
		return nullptr;
	}

	void Log::Initialize()
	{
		s_Data = sknew LogData();

		s_ConsoleSink = std::make_shared<ConsoleSink>();

		std::vector<spdlog::sink_ptr> sharkSinks =
		{
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/Core.log", true)
		};

		std::vector<spdlog::sink_ptr> clientSinks =
		{
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/App.log", true),
			s_ConsoleSink
		};

		std::vector<spdlog::sink_ptr> consoleSinks =
		{
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/App.log", true),
			s_ConsoleSink
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
		s_ConsoleSink = nullptr;
		skdelete s_Data;
		s_Data = nullptr;
	}

	void Log::SetConsoleSinkCallback(std::function<void(ConsoleSinkMessage&&)> callback)
	{
		s_ConsoleSink->SetPushMessageCallback(callback);
	}

}