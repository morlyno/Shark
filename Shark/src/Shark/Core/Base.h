#pragma once

#if !SK_PLATFORM_WINDOWS
	#error Platform other than Windows are currently not supported
#endif

#if (!defined(SK_RELEASE) && !defined(SK_DEBUG)) || (defined(SK_RELEASE) && defined(SK_DEBUG))
	#error Invalid Configuration
#endif

#if defined(_MSC_VER)
	#define SK_COMPILER_MSVC (1)
#else
	#error Compiler not supported
#endif

#if defined(SK_COMPILER_MSVC)
	#define SK_FUNCTION_NAME __func__
	#define SK_FUNCTION_DECORATED __FUNCTION__
	#define SK_FUNCTION_SIGNATURE __PRETTY_FUNCTION__

	#define SK_FILE __FILE__
	#define SK_LINE __LINE__
#endif


#if SK_DEBUG
	#define SK_DEBUG_BREAK() __debugbreak()
	#define SK_TRACK_MEMORY 1
	#define SK_ENABLE_ASSERT 1
	#define SK_ENABLE_VERIFY 1
	#define SK_ENABLE_GPU_VALIDATION 1
	#define SK_ENABLE_PERF 1
	#define SK_ENABLE_PROFILER 0
#endif

#if SK_RELEASE
	#define SK_DEBUG_BREAK() __debugbreak()
	#define SK_TRACK_MEMORY 1
	#define SK_ENABLE_ASSERT 0
	#define SK_ENABLE_VERIFY 1
	#define SK_ENABLE_GPU_VALIDATION 1
	#define SK_ENABLE_PERF 1
	#define SK_ENABLE_PROFILER 1
#endif

#define BIT(x) (1 << x)

#define SK_STRINGIFY(x) #x
#define SK_EXPAND(x) x
#define SK_CONNECT(a, b) INTERNAL_CONNECT(a, b)
#define INTERNAL_CONNECT(a, b) a ## b

#define SK_DEBUG_BREAK_CONDITIONAL(_cond_var_name) static bool _cond_var_name = true; if (_cond_var_name) { SK_DEBUG_BREAK(); }
#define SK_BIND_EVENT_FN(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }

#define SK_NOT_IMPLEMENTED() SK_CORE_ERROR("Not Implemented!"); SK_DEBUG_BREAK()
#define SK_DEPRECATED(message) [[deprecated(message)]]

#ifdef SK_COMPILER_MSVC
	#define GET_ARG_COUNT(...)  INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))

	#define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
	#define INTERNAL_EXPAND(x) x
	#define INTERNAL_EXPAND_ARGS_PRIVATE(...) INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
	#define INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count
#else
	#define GET_ARG_COUNT(...) INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
	#define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count
#endif

#include <stdint.h>
#include <magic_enum.hpp>

namespace Shark {

	using byte = unsigned char;

	using RenderID = void*;
	using WindowHandle = void*;

	using namespace std::literals;
	using namespace magic_enum::bitwise_operators;

}

#include "Shark/Core/Memory.h"
#include "Shark/Core/Log.h"
#include "Shark/Core/Assert.h"

#include "Shark/Core/RefCount.h"
#include "Shark/Core/Scope.h"

