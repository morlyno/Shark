#pragma once

#ifdef SK_ENABLE_ASSERT
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
