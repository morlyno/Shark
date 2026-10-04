#include "skpch.h"
#include "Log.h"

#include "Shark/Core/Project.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/dist_sink.h>
#include <stacktrace>

namespace Shark {

	static_assert(std::to_underlying(Log::Level::Trace)    == std::to_underlying(spdlog::level::trace));
	static_assert(std::to_underlying(Log::Level::Debug)    == std::to_underlying(spdlog::level::debug));
	static_assert(std::to_underlying(Log::Level::Info)     == std::to_underlying(spdlog::level::info));
	static_assert(std::to_underlying(Log::Level::Warning)  == std::to_underlying(spdlog::level::warn));
	static_assert(std::to_underlying(Log::Level::Error)    == std::to_underlying(spdlog::level::err));
	static_assert(std::to_underlying(Log::Level::Critical) == std::to_underlying(spdlog::level::critical));
	static_assert(std::to_underlying(Log::Level::Off)      == std::to_underlying(spdlog::level::off));

	static std::unique_ptr<Logging> s_Logging = nullptr;

	static constexpr Log::TagArray<Log::Level> s_DefaultTagLevels = []()
	{
		Log::TagArray<Log::Level> levels;

		static constexpr auto start = std::source_location::current().line() + 1;
		levels[Log::Tag::Default]        = Log::Level::Trace;
		levels[Log::Tag::AssetManager]   = Log::Level::Warning;
		levels[Log::Tag::AssetThread]    = Log::Level::Warning;
		levels[Log::Tag::Assimp]         = Log::Level::Error;
		levels[Log::Tag::Audio]          = Log::Level::Info;
		levels[Log::Tag::Core]           = Log::Level::Trace;
		levels[Log::Tag::Editor]         = Log::Level::Trace;
		levels[Log::Tag::Filesystem]     = Log::Level::Warning;
		levels[Log::Tag::Font]           = Log::Level::Info;
		levels[Log::Tag::ImGui]          = Log::Level::Trace;
		levels[Log::Tag::MiniAudio]      = Log::Level::Error;
		levels[Log::Tag::NVRHI]          = Log::Level::Warning;
		levels[Log::Tag::Renderer]       = Log::Level::Warning;
		levels[Log::Tag::Scene]          = Log::Level::Info;
		levels[Log::Tag::Scripting]      = Log::Level::Info;
		levels[Log::Tag::Serialization]  = Log::Level::Warning;
		levels[Log::Tag::ShaderCompiler] = Log::Level::Warning;
		levels[Log::Tag::stbi]           = Log::Level::Trace;
		levels[Log::Tag::ThumbnailCache] = Log::Level::Info;
		levels[Log::Tag::Timer]          = Log::Level::Off;
		levels[Log::Tag::Utilities]      = Log::Level::Warning;
		levels[Log::Tag::Window]         = Log::Level::Trace;
		levels[Log::Tag::Windows]        = Log::Level::Trace;
		static constexpr auto end = std::source_location::current().line();
		static_assert(end - start == magic_enum::enum_count<Log::Tag>());

		return levels;
	}();

	Logging::Logging()
	{
		auto stdoutSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		auto coreFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("Logs/Core.log", std::numeric_limits<size_t>::max(), 4, true);
		auto userFileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("Logs/App.log", std::numeric_limits<size_t>::max(), 2, true);

		stdoutSink->set_pattern("%^[%T] %n: %v%$");
		coreFileSink->set_pattern("[%c] [%l] %n: %v");
		userFileSink->set_pattern("[%c] [%l] %n: %v");

		spdlog::sinks_init_list core = { stdoutSink, coreFileSink };

		auto user = std::make_shared<spdlog::sinks::dist_sink_mt>();
		user->add_sink(stdoutSink);
		user->add_sink(userFileSink);

		m_Core = std::make_shared<spdlog::logger>("SHARK", core);
		m_User = std::make_shared<spdlog::logger>("APP", user);

		m_Core->set_level(spdlog::level::trace);
		m_User->set_level(spdlog::level::trace);

		// spdlog::set_global_logger(...);
		SetDefaultTagLevels();
	}

	Logging::~Logging()
	{
		spdlog::shutdown();
	}

	bool Logging::ShouldLog(Log::Level level, Log::Tag tag)
	{
		return level >= m_Levels[tag];
	}

	void Logging::CoreLog(Log::Level level, Log::Tag tag, std::string_view msg)
	{
		if (!ShouldLog(level, tag))
			return;

		if (tag == Log::Tag::Default)
		{
			m_Core->log(static_cast<spdlog::level>(level), msg);
		}
		else
		{
			m_Core->log(static_cast<spdlog::level>(level), "[{}] {}", tag, msg);
		}
	}

	void Logging::UserLog(Log::Level level, std::string_view msg)
	{
		m_User->log(static_cast<spdlog::level>(level), msg);
	}

	void Logging::AssertMessage(const Log::AssertInfo& info)
	{
		if (info.Message.empty())
		{
			m_Core->log(static_cast<spdlog::level>(info.Level),
						"{} on '{}' {}\n{}",
						info.Prefix,
						info.Condition,
						info.Location,
						std::to_string(info.Stacktrace));
		}
		else
		{
			m_Core->log(static_cast<spdlog::level>(info.Level),
						"{} on '{}' {}: {}\n{}",
						info.Prefix,
						info.Condition,
						info.Location,
						info.Message,
						std::to_string(info.Stacktrace));
		}
	}

	std::shared_ptr<spdlog::logger> Logging::CoreLogger() const
	{
		return m_Core;
	}

	std::shared_ptr<spdlog::logger> Logging::UserLogger() const
	{
		return m_User;
	}

	void Logging::AddUserSink(spdlog::sink_ptr sink)
	{
		static_cast<spdlog::sinks::dist_sink_mt*>(m_User->sinks().front().get())->add_sink(sink);
	}

	void Logging::RemoveUserSink(spdlog::sink_ptr sink)
	{
		static_cast<spdlog::sinks::dist_sink_mt*>(m_User->sinks().front().get())->remove_sink(sink);
	}

	void Logging::SetDefaultTagLevels()
	{
		m_Levels = s_DefaultTagLevels;
	}

	Log::Level Logging::GetDefaultLevel(Log::Tag tag) const
	{
		return s_DefaultTagLevels[tag];
	}

	void Logging::SetDefaultLevel(Log::Tag tag)
	{
		m_Levels[tag] = s_DefaultTagLevels[tag];
	}

	Log::Level Logging::GetLevel(Log::Tag tag) const
	{
		return m_Levels[tag];
	}

	void Logging::SetLevel(Log::Tag tag, Log::Level level)
	{
		m_Levels[tag] = level;
	}

	void Logging::ApplyProjectConfig(const ProjectConfig* config)
	{
		SetDefaultTagLevels();

		// config is null when project is closed
		if (!config)
			return;

		for (const auto& [tag, level] : config->CustomLogLevels)
		{
			m_Levels[tag] = level;
		}
	}

	void Log::Initialize()
	{
		s_Logging = std::make_unique<Logging>();
	}

	void Log::Shutdown()
	{
		s_Logging = nullptr;
	}

	Logging* Log::Get()
	{
		return s_Logging.get();
	}

}
