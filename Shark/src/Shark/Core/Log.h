#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Enum.h"

#include "Shark/Utils/fmtUtils.h"

#include <spdlog/spdlog.h>

#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/os.h>
#include <fmt/ranges.h>
#include <fmt/std.h>

#include <magic_enum_format.hpp>

#include <source_location>
#include <stacktrace>

namespace Shark {

	namespace Log {

		enum class Level : uint16_t
		{
			Trace    = 0,
			Debug    = 1,
			Info     = 2,
			Warning  = 3,
			Error    = 4,
			Critical = 5,
			Off      = 6
		};

		enum class Tag : uint16_t
		{
			Default = 0,

			AssetManager,
			AssetThread,
			Assimp,
			Audio,
			Core,
			Editor,
			Filesystem,
			Font,
			ImGui,
			MiniAudio,
			NVRHI,
			Renderer,
			Scene,
			Scripting,
			Serialization,
			ShaderCompiler,
			stbi,
			ThumbnailCache,
			Timer,
			Utilities,
			Window,
			Windows
		};

		template<typename T>
		using TagArray = Enum::Array<Tag, T>;

		struct AssertInfo
		{
			Level Level = Level::Error;
			std::string_view Prefix;
			std::string_view Condition;
			std::string Message;

			std::source_location Location;
			std::stacktrace Stacktrace;

			static AssertInfo Current(Log::Level level, std::string_view prefix, std::string_view condition, const std::source_location& location = std::source_location::current(), std::stacktrace stacktrace = std::stacktrace::current())
			{
				return AssertInfo()
					.SetLevel(level)
					.SetPrefix(prefix)
					.SetCondition(condition)
					.SetLocation(location)
					.SetStacktrace(std::move(stacktrace));
			}

			AssertInfo& SetLevel(Log::Level level) { Level = level; return *this; }
			AssertInfo& SetPrefix(std::string_view prefix) { Prefix = prefix; return *this; }
			AssertInfo& SetCondition(std::string_view condition) { Condition = condition; return *this; }
			AssertInfo& SetMessage(std::string message) { Message = std::move(message); return *this; }
			AssertInfo& SetLocation(const std::source_location& location = std::source_location::current()) { Location = location; return *this; }
			AssertInfo& SetStacktrace(std::stacktrace stacktrace) { Stacktrace = std::move(stacktrace); return *this; }
			AssertInfo& SetStacktrace(size_t skip = 0) { Stacktrace = std::stacktrace::current(skip); return *this; }

			template<typename... Args>
			AssertInfo& SetMessage(fmt::format_string<Args...> fmt, Args&&... args)
			{
				return SetMessage(fmt::format(fmt, std::forward<Args>(args)...));
			}
		};

	}

	class Logging
	{
	public:
		Logging();
		~Logging();

		bool ShouldLog(Log::Level level, Log::Tag tag);

		template<typename... Args>
		void CoreLog(Log::Level level, Log::Tag tag, fmt::format_string<Args...> fmt, Args&&... args);
		void CoreLog(Log::Level level, Log::Tag tag, std::string_view msg);

		template<typename... Args>
		void UserLog(Log::Level level, fmt::format_string<Args...> fmt, Args&&... args);
		void UserLog(Log::Level level, std::string_view msg);

		void AssertMessage(const Log::AssertInfo& info);

	public:
		std::shared_ptr<spdlog::logger> CoreLogger() const;
		std::shared_ptr<spdlog::logger> UserLogger() const;

		void AddUserSink(spdlog::sink_ptr sink);
		void RemoveUserSink(spdlog::sink_ptr sink);

		void SetDefaultTagLevels();
		Log::Level GetDefaultLevel(Log::Tag tag) const;
		void SetDefaultLevel(Log::Tag tag);

		Log::Level GetLevel(Log::Tag tag) const;
		void SetLevel(Log::Tag tag, Log::Level level);

	private:
		std::shared_ptr<spdlog::logger> m_Core;
		std::shared_ptr<spdlog::logger> m_User;
		
		Log::TagArray<Log::Level> m_Levels;
	};

	namespace Log {

		void Initialize();
		void Shutdown();

		Logging* Get();

		template<typename... Args> void CoreTrace(Tag tag, fmt::format_string<Args...> fmt, Args&&... args)		{ Get()->CoreLog(Level::Trace,    tag, fmt, std::forward<Args>(args)...); }
		template<typename... Args> void CoreDebug(Tag tag, fmt::format_string<Args...> fmt, Args&&... args)		{ Get()->CoreLog(Level::Debug,    tag, fmt, std::forward<Args>(args)...); }
		template<typename... Args> void CoreInfo(Tag tag, fmt::format_string<Args...> fmt, Args&&... args)		{ Get()->CoreLog(Level::Info,     tag, fmt, std::forward<Args>(args)...); }
		template<typename... Args> void CoreWarning(Tag tag, fmt::format_string<Args...> fmt, Args&&... args)	{ Get()->CoreLog(Level::Warning,  tag, fmt, std::forward<Args>(args)...); }
		template<typename... Args> void CoreError(Tag tag, fmt::format_string<Args...> fmt, Args&&... args)		{ Get()->CoreLog(Level::Error,    tag, fmt, std::forward<Args>(args)...); }
		template<typename... Args> void CoreCritical(Tag tag, fmt::format_string<Args...> fmt, Args&&... args)	{ Get()->CoreLog(Level::Critical, tag, fmt, std::forward<Args>(args)...); }

		inline void CoreTrace(Tag tag, std::string_view message)												{ Get()->CoreLog(Level::Trace,    tag, message); }
		inline void CoreDebug(Tag tag, std::string_view message)												{ Get()->CoreLog(Level::Debug,    tag, message); }
		inline void CoreInfo(Tag tag, std::string_view message)													{ Get()->CoreLog(Level::Info,     tag, message); }
		inline void CoreWarning(Tag tag, std::string_view message)												{ Get()->CoreLog(Level::Warning,  tag, message); }
		inline void CoreError(Tag tag, std::string_view message)												{ Get()->CoreLog(Level::Error,    tag, message); }
		inline void CoreCritical(Tag tag, std::string_view message)												{ Get()->CoreLog(Level::Critical, tag, message); }

		template<typename... Args> void UserTrace(fmt::format_string<Args...> fmt, Args&&... args)				{ Get()->UserLog(Level::Trace,    fmt, std::forward<Args>(args)...); }
		template<typename... Args> void UserDebug(fmt::format_string<Args...> fmt, Args&&... args)				{ Get()->UserLog(Level::Debug,    fmt, std::forward<Args>(args)...); }
		template<typename... Args> void UserInfo(fmt::format_string<Args...> fmt, Args&&... args)				{ Get()->UserLog(Level::Info,     fmt, std::forward<Args>(args)...); }
		template<typename... Args> void UserWarning(fmt::format_string<Args...> fmt, Args&&... args)			{ Get()->UserLog(Level::Warning,  fmt, std::forward<Args>(args)...); }
		template<typename... Args> void UserError(fmt::format_string<Args...> fmt, Args&&... args)				{ Get()->UserLog(Level::Error,    fmt, std::forward<Args>(args)...); }
		template<typename... Args> void UserCritical(fmt::format_string<Args...> fmt, Args&&... args)			{ Get()->UserLog(Level::Critical, fmt, std::forward<Args>(args)...); }
		
		inline void UserTrace(std::string_view message)															{ Get()->UserLog(Level::Trace,    message); }
		inline void UserDebug(std::string_view message)															{ Get()->UserLog(Level::Debug,    message); }
		inline void UserInfo(std::string_view message)															{ Get()->UserLog(Level::Info,     message); }
		inline void UserWarning(std::string_view message)														{ Get()->UserLog(Level::Warning,  message); }
		inline void UserError(std::string_view message)															{ Get()->UserLog(Level::Error,    message); }
		inline void UserCritical(std::string_view message)														{ Get()->UserLog(Level::Critical, message); }

	}

}

namespace Shark {

	template<typename... Args>
	void Logging::CoreLog(Log::Level level, Log::Tag tag, fmt::format_string<Args...> fmt, Args&&... args)
	{
		if (!ShouldLog(level, tag))
			return;

		if (tag == Log::Tag::Default)
		{
			m_Core->log(static_cast<spdlog::level>(level), fmt, std::forward<Args>(args)...);
		}
		else if (m_Core->should_log(static_cast<spdlog::level>(level)))
		{
			m_Core->log(static_cast<spdlog::level>(level),
						"[{}] {}",
						tag,
						fmt::format(fmt, std::forward<Args>(args)...));
		}
	}

	template<typename... Args>
	void Logging::UserLog(Log::Level level, fmt::format_string<Args...> fmt, Args&&... args)
	{
		m_User->log(static_cast<spdlog::level>(level), fmt, std::forward<Args>(args)...);
	}

}

#define SK_CORE_DEBUG(...)				::Shark::Log::CoreDebug(::Shark::Log::Tag::Default, __VA_ARGS__)

#define SK_CORE_TRACE_TAG(_tag, ...)	::Shark::Log::CoreTrace(_tag, __VA_ARGS__)
#define SK_CORE_DEBUG_TAG(_tag, ...)	::Shark::Log::CoreDebug(_tag, __VA_ARGS__)
#define SK_CORE_INFO_TAG(_tag, ...)		::Shark::Log::CoreInfo(_tag, __VA_ARGS__)
#define SK_CORE_WARNING_TAG(_tag, ...)	::Shark::Log::CoreWarning(_tag, __VA_ARGS__)
#define SK_CORE_ERROR_TAG(_tag, ...)	::Shark::Log::CoreError(_tag, __VA_ARGS__)
#define SK_CORE_CRITICAL_TAG(_tag, ...)	::Shark::Log::CoreCritical(_tag, __VA_ARGS__)

#define SK_USER_TRACE(...)				::Shark::Log::UserTrace(__VA_ARGS__)
#define SK_USER_DEBUG(...)				::Shark::Log::UserDebug(__VA_ARGS__)
#define SK_USER_INFO(...)				::Shark::Log::UserInfo(__VA_ARGS__)
#define SK_USER_WARNING(...)			::Shark::Log::UserWarning(__VA_ARGS__)
#define SK_USER_ERROR(...)				::Shark::Log::UserError(__VA_ARGS__)
#define SK_USER_CRITICAL(...)			::Shark::Log::UserCritical(__VA_ARGS__)

#if SK_ENABLE_ASSERT
	#define SK_CORE_ASSERT(_condition, ...)																		\
		if (!(_condition))																						\
		{																										\
			::Shark::Log::Get()->AssertMessage(																	\
				::Shark::Log::AssertInfo::Current(::Shark::Log::Level::Error, "Assertion failed", #_condition)	\
					__VA_OPT__(.SetMessage(__VA_ARGS__))														\
			);																									\
		}
#else
	#define SK_CORE_ASSERT(...) (void)0
#endif

#if SK_ENABLE_VERIFY
	#define SK_CORE_VERIFY(_condition, ...)																		\
		if (!(_condition))																						\
		{																										\
			::Shark::Log::Get()->AssertMessage(																	\
				::Shark::Log::AssertInfo::Current(::Shark::Log::Level::Critical, "Verify failed", #_condition)	\
					__VA_OPT__(.SetMessage(__VA_ARGS__))														\
			);																									\
		}
#else
	#define SK_CORE_VERFIY(...) (void)0
#endif
