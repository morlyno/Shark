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
	#define SK_ENABLE_VALIDATION 1
	#define SK_ENABLE_PERF 1
	#define SK_ENABLE_PROFILER 0
#endif

#if SK_RELEASE
	#define SK_DEBUG_BREAK() __debugbreak()
	#define SK_TRACK_MEMORY 1
	#define SK_ENABLE_ASSERT 0
	#define SK_ENABLE_VERIFY 1
	#define SK_ENABLE_VALIDATION 1
	#define SK_ENABLE_PERF 1
	#define SK_ENABLE_PROFILER 1
#endif

#define SK_DEBUG_BREAK_CONDITIONAL(_cond_var_name) static bool _cond_var_name = true; if (_cond_var_name) { SK_DEBUG_BREAK(); }

#pragma region Macro Internal

#define SK_IMPL_CONNECT(a, b) a##b

#define SK_IMPL_UNIQUE_NAME_BASE_CONNECT(a, b) a ## b
#define SK_IMPL_UNIQUE_NAME_BASE(x) SK_IMPL_UNIQUE_NAME_BASE_CONNECT(unique_name_, x)

#define INTERNAL_SK_ARG_N(\
_1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
_11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
_21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
_31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
_41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
_51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
_61,_62,_63,N,...) N

#define INTERNAL_SK_RSEQ_N() \
63,62,61,60,                   \
59,58,57,56,55,54,53,52,51,50, \
49,48,47,46,45,44,43,42,41,40, \
39,38,37,36,35,34,33,32,31,30, \
29,28,27,26,25,24,23,22,21,20, \
19,18,17,16,15,14,13,12,11,10, \
9,8,7,6,5,4,3,2,1,0

#define INTERNAL_CONNECT_STRINGIFY2(a) #a
#define INTERNAL_CONNECT_STRINGIFY(a, b) INTERNAL_CONNECT_STRINGIFY2(a##b)

#pragma endregion

#define BIT(x) (1 << x)
#define SET_BIT(_bit, _value) (0b ## _value << _bit)

#define SK_STRINGIFY(x) #x
#define SK_EXPAND(x) x
#define SK_CONNECT(a, b) SK_IMPL_CONNECT(a, b)
#define SK_CONNECT_STRINGIFY(a, b) INTERNAL_CONNECT_STRINGIFY(a, b)

#define SK_BIND_EVENT_FN(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }

#define SK_NOT_IMPLEMENTED() SK_CORE_ERROR("Not Implemented!"); SK_DEBUG_BREAK()
#define SK_DEPRECATED(message) [[deprecated(message)]]

#define SK_UNIQUE_NAME SK_IMPL_UNIQUE_NAME_BASE(__COUNTER__)

#define SK_NUM_ARGS(...) INTERNAL_SK_ARG_N(__VA_ARGS__,INTERNAL_SK_RSEQ_N())

#include <stdint.h>
#include <magic_enum.hpp>

namespace Shark {

	using byte = unsigned char;

	using RenderID = void*;
	using WindowHandle = void*;

	using namespace std::literals::string_literals;
	using namespace std::literals::string_view_literals;
	using namespace std::literals::chrono_literals;
	using namespace magic_enum::bitwise_operators;

}

#include "Shark/Core/Memory.h"
#include "Shark/Core/Log.h"
#include "Shark/Core/Assert.h"

#include "Shark/Core/RefCount.h"
#include "Shark/Core/Scope.h"

