#pragma once

#if SK_ENABLE_ASSERT
#define SK_INTERNAL_ASSERT_IMPL(type, arg, msg, ...) if (!(arg)) { SK##type##ERROR(msg, __VA_ARGS__); SK_DEBUG_BREAK(); }
#define SK_INTERNAL_ASSERT_MSG(type, arg, ...) SK_INTERNAL_ASSERT_IMPL(type, arg,"Assertion {0} Failed: {1}", SK_STRINGIFY(arg), __VA_ARGS__)
#define SK_INTERNAL_ASSERT_NO_MSG(type, arg, ...) SK_INTERNAL_ASSERT_IMPL(type, arg, "Assertion {0} Failed at {1}:{2}",SK_STRINGIFY(arg) ,__FILE__,__LINE__)

#define SK_INTERNAL_ASSERT_GET_MACRO_NAME(arg0, arg1, macro, ...) macro
#define SK_INTERNAL_ASSERT_GET_MACRO(...) SK_EXPAND( SK_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, SK_INTERNAL_ASSERT_MSG, SK_INTERNAL_ASSERT_NO_MSG ) )

#define SK_ASSERT(...) SK_EXPAND( SK_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
#define SK_CORE_ASSERT(...) SK_EXPAND( SK_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
#define SK_ASSERT(...)
#define SK_CORE_ASSERT(...)
#endif

#if SK_ENABLE_VERIFY == 1
#define SK_INTERNAL_VERIFY_IMPL(arg, msg) if (!(arg)) { SK_CORE_ERROR(msg); SK_DEBUG_BREAK(); }
#elif SK_ENABLE_VERIFY == 2
#define SK_INTERNAL_VERIFY_IMPL(arg, msg) if (!(arg)) { SK_CORE_ERROR(msg); }
#endif

#if (SK_ENABLE_VERIFY > 0)
#define SK_INTERNAL_VERIFY_MSG(arg, msg) SK_INTERNAL_VERIFY_IMPL(arg, fmt::format("Verify {} Failed: {}", SK_STRINGIFY(arg), msg))
#define SK_INTERNAL_VERIFY_NO_MSG(arg, ...) SK_INTERNAL_VERIFY_IMPL(arg, fmt::format("Verify {} Failed at {}:{}", SK_STRINGIFY(arg), __FILE__, __LINE__))
#define SK_INTERNAL_VERIFY_GET_MACRO_2(arg0, arg1, macro, ...) macro
#define SK_INTERNAL_VERIFY_GET_MACRO(...) SK_EXPAND(SK_INTERNAL_VERIFY_GET_MACRO_2(__VA_ARGS__, SK_INTERNAL_VERIFY_MSG, SK_INTERNAL_VERIFY_NO_MSG))

#define SK_CORE_VERIFY(...) SK_EXPAND(SK_INTERNAL_VERIFY_GET_MACRO(__VA_ARGS__)(__VA_ARGS__))
#endif
