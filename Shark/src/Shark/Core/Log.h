#pragma once

#include "Core.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace Shark {

	class Log
	{
	public:
		static void Init();

		static std::shared_ptr<spdlog::logger> GetCoreLogger() { return s_Core_Logger; }
		static std::shared_ptr<spdlog::logger> GetClientLogger() { return s_Client_Logger; }
	private:
		static std::shared_ptr<spdlog::logger> s_Core_Logger;
		static std::shared_ptr<spdlog::logger> s_Client_Logger;
	};

}

#define SK_CORE_TRACE(...)		::Shark::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define SK_CORE_INFO(...)		::Shark::Log::GetCoreLogger()->info(__VA_ARGS__)
#define SK_CORE_WARN(...)		::Shark::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define SK_CORE_ERROR(...)		::Shark::Log::GetCoreLogger()->error(__VA_ARGS__)
#define SK_CORE_CRITICAL(...)	::Shark::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define SK_TRACE(...)			::Shark::Log::GetClientLogger()->trace(__VA_ARGS__)
#define SK_INFO(...)			::Shark::Log::GetClientLogger()->info(__VA_ARGS__)
#define SK_WARN(...)			::Shark::Log::GetClientLogger()->warn(__VA_ARGS__)
#define SK_ERROR(...)			::Shark::Log::GetClientLogger()->error(__VA_ARGS__)
#define SK_CRITICAL(...)		::Shark::Log::GetClientLogger()->critical(__VA_ARGS__)

#ifdef SK_DEBUG
#define SK_CORE_DEBUG(...)		::Shark::Log::GetCoreLogger()->debug(__VA_ARGS__)
#else
#define SK_CORE_DEBUG(...)
#endif
