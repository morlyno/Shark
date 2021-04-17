#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace Shark {

	class Log
	{
	private:
		static std::shared_ptr<spdlog::logger> InitPtrCore() { Log::Init(); return s_Core_Logger; }
		static std::shared_ptr<spdlog::logger> InitPtrClient() { Log::Init(); return s_Client_Logger; }

		static std::shared_ptr<spdlog::logger> GetCoreLoggerImpl() { return s_Core_Logger; }
		static std::shared_ptr<spdlog::logger> GetClientLoggerImpl() { return s_Client_Logger; }

	public:
		static void Init();

		static std::shared_ptr<spdlog::logger>(*GetCoreLogger)();
		static std::shared_ptr<spdlog::logger>(*GetClientLogger)();

	private:
		static std::shared_ptr<spdlog::logger> s_Core_Logger;
		static std::shared_ptr<spdlog::logger> s_Client_Logger;
	};

}

#ifndef SK_DISABLE_LOGGING
#define SK_CORE_TRACE(...)		::Shark::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define SK_CORE_INFO(...)		::Shark::Log::GetCoreLogger()->info(__VA_ARGS__)
#define SK_CORE_WARN(...)		::Shark::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define SK_CORE_ERROR(...)		::Shark::Log::GetCoreLogger()->error(__VA_ARGS__)
#define SK_CORE_CRITICAL(...)	::Shark::Log::GetCoreLogger()->critical(__VA_ARGS__)
#define SK_CORE_DEBUG(...)		::Shark::Log::GetCoreLogger()->debug(__VA_ARGS__)

#define SK_TRACE(...)			::Shark::Log::GetClientLogger()->trace(__VA_ARGS__)
#define SK_INFO(...)			::Shark::Log::GetClientLogger()->info(__VA_ARGS__)
#define SK_WARN(...)			::Shark::Log::GetClientLogger()->warn(__VA_ARGS__)
#define SK_ERROR(...)			::Shark::Log::GetClientLogger()->error(__VA_ARGS__)
#define SK_CRITICAL(...)		::Shark::Log::GetClientLogger()->critical(__VA_ARGS__)
#else
#define SK_CORE_TRACE(...)
#define SK_CORE_INFO(...)
#define SK_CORE_WARN(...)
#define SK_CORE_ERROR(...)
#define SK_CORE_CRITICAL(...)
#define SK_CORE_DEBUG(...)

#define SK_TRACE(...)
#define SK_INFO(...)
#define SK_WARN(...)
#define SK_ERROR(...)
#define SK_CRITICAL(...)
#endif
