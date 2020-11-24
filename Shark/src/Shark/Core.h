#pragma once

#ifdef SK_PLATFORM_WINDOWS
	#ifdef SK_BUILD_DLL
		#define SHARK_API __declspec(dllexport)
	#else
		#define SHARK_API __declspec(dllimport)
	#endif
#else
	#error Shark only supports Windows
#endif

#define SK_CORE_LOG_TRACE(...)		::Shark::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define SK_CORE_LOG_INFO(...)		::Shark::Log::GetCoreLogger()->info(__VA_ARGS__)
#define SK_CORE_LOG_WARN(...)		::Shark::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define SK_CORE_LOG_ERROR(...)		::Shark::Log::GetCoreLogger()->error(__VA_ARGS__)
#define SK_CORE_LOG_CRITICAL(...)	::Shark::Log::GetCoreLogger()->critical(__VA_ARGS__)
#define SK_CORE_LOG_DEBUG(...)		::Shark::Log::GetCoreLogger()->debug(__VA_ARGS__)

#define SK_CLIENT_LOG_TRACE(...)	::Shark::Log::GetClientLogger()->trace(__VA_ARGS__)
#define SK_CLIENT_LOG_INFO(...)		::Shark::Log::GetClientLogger()->info(__VA_ARGS__)
#define SK_CLIENT_LOG_WARN(...)		::Shark::Log::GetClientLogger()->warn(__VA_ARGS__)
#define SK_CLIENT_LOG_ERROR(...)	::Shark::Log::GetClientLogger()->error(__VA_ARGS__)
#define SK_CLIENT_LOG_CRITICAL(...)	::Shark::Log::GetClientLogger()->critical(__VA_ARGS__)
#define SK_CLIENT_LOG_DEBUG(...)	::Shark::Log::GetClientLogger()->debug(__VA_ARGS__)

#define BIT(x) (1 << x)