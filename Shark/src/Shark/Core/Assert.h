#pragma once

#if SK_ENABLE_ASSERT
#define SK_CORE_ASSERT_MESSAGE_INTERNAL(...) ::Shark::Log::PrintAssertMessage(::Shark::Log::Logger::Core, "Assertion Failed", __VA_ARGS__);
#define SK_ASSERT_MESSAGE_INTERNAL(...) ::Shark::Log::PrintAssertMessage(::Shark::Log::Logger::Client, "Assertion Failed", __VA_ARGS__);

#define SK_CORE_ASSERT(condition, ...) if (!(condition)) { SK_CORE_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); SK_DEBUG_BREAK(); }
#define SK_ASSERT(condition, ...) if (!(condition)) { SK_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); SK_DEBUG_BREAK(); }
#else
#define SK_ASSERT(...)
#define SK_CORE_ASSERT(...)
#endif

#if SK_ENABLE_VERIFY
#define SK_CORE_VERIFY_MESSAGE_INTERNAL(...) ::Shark::Log::PrintAssertMessage(::Shark::Log::Logger::Core, "Verify Failed", __VA_ARGS__);
#define SK_VERIFY_MESSAGE_INTERNAL(...) ::Shark::Log::PrintAssertMessage(::Shark::Log::Logger::Client, "Verify Failed", __VA_ARGS__);

#define SK_CORE_VERIFY(condition, ...) if (!(condition)) { SK_CORE_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); SK_DEBUG_BREAK(); }
#define SK_VERIFY(condition, ...) if (!(condition)) { SK_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); SK_DEBUG_BREAK(); }
#else
#define SK_CORE_VERIFY(...)
#define SK_VERIFY(...)
#endif
