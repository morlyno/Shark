#include "skpch.h"
#include "Log.h"

#include "Shark/Core/ConsoleSink.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace Shark {

	static std::shared_ptr<ConsoleSink> s_ConsoleSink = nullptr;

	std::shared_ptr<spdlog::logger> Log::GetLogger(LoggerType loggerType)
	{
		if (loggerType == LoggerType::Core)
			return s_CoreLogger;

		else if (loggerType == LoggerType::Client)
			return s_ClientLogger;

		else if (loggerType == LoggerType::Console)
			return s_ConsoleLogger;

		SK_CORE_ASSERT(false, "Unkown LoggerType");
		return nullptr;
	}

	void Log::PrintMessage(LoggerType loggerType, LogLevel level, std::string_view message)
	{
		auto& settings = s_EnabledTags[""];
		if (settings.Enabled && level >= settings.Level)
		{
			auto logger = GetLogger(loggerType);
			switch (level)
			{
				case LogLevel::Trace: logger->trace(message); break;
				case LogLevel::Debug: logger->debug(message); break;
				case LogLevel::Info: logger->info(message); break;
				case LogLevel::Warn: logger->warn(message); break;
				case LogLevel::Error: logger->error(message); break;
				case LogLevel::Critical: logger->critical(message); break;
			}
		}
	}

	void Log::PrintMessageTag(LoggerType loggerType, LogLevel level, std::string_view tag, std::string_view message)
	{
		auto& settings = s_EnabledTags[std::string(tag)];
		if (settings.Enabled && level >= settings.Level)
		{
			auto logger = GetLogger(loggerType);
			switch (level)
			{
				case LogLevel::Trace: logger->trace("[{}] {}", tag, message); break;
				case LogLevel::Debug: logger->debug("[{}] {}", tag, message); break;
				case LogLevel::Info: logger->info("[{}] {}", tag, message); break;
				case LogLevel::Warn: logger->warn("[{}] {}", tag, message); break;
				case LogLevel::Error: logger->error("[{}] {}", tag, message); break;
				case LogLevel::Critical: logger->critical("[{}] {}", tag, message); break;
			}
		}
	}

	void Log::Initialize()
	{
		s_ConsoleSink = std::make_shared<ConsoleSink>();
		auto stdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		auto coreFileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/Core.log", true);
		auto clientFileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/App.log", true);

		stdoutSink->set_pattern("%^[%T] %n: %v%$");
		coreFileSink->set_pattern("[%c] [%l] %n: %v");
		clientFileSink->set_pattern("%^[%T] %n: %v%$");

		spdlog::sinks_init_list sharkSinks =
		{
			stdoutSink,
			coreFileSink
		};

		spdlog::sinks_init_list clientSinks =
		{
			stdoutSink,
			clientFileSink,
			s_ConsoleSink
		};

		spdlog::sinks_init_list consoleSinks =
		{
			clientFileSink,
			s_ConsoleSink
		};

		s_CoreLogger = std::make_shared<spdlog::logger>("SHARK", sharkSinks);
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = std::make_shared<spdlog::logger>("APP", clientSinks);
		s_ClientLogger->set_level(spdlog::level::trace);

		s_ConsoleLogger = std::make_shared<spdlog::logger>("APP", consoleSinks);
		s_ConsoleLogger->set_level(spdlog::level::trace);
	}

	void Log::Shutdown()
	{
		s_ConsoleSink.reset();

		s_CoreLogger.reset();
		s_ClientLogger.reset();
		s_ConsoleLogger.reset();

		s_EnabledTags.clear();
	}

	void Log::SetConsoleSinkCallback(std::function<void(ConsoleSinkMessage&&)> callback)
	{
		s_ConsoleSink->SetPushMessageCallback(callback);
	}

	spdlog::sink_ptr Log::GetConsoleSink()
	{
		return s_ConsoleSink;
	}

}