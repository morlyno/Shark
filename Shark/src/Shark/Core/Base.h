#pragma once

#if !SK_PLATFORM_WINDOWS
	#error Platform other than Windows are currently not supported
#endif


#if SK_DEBUG
	#define SK_DEBUG_BREAK() __debugbreak()
	#define SK_ENABLE_MEMORY_TRACING 0
	#define SK_ENABLE_ASSERT 1
	#define SK_ENABLE_VERIFY 1
	#define SK_IF_DEBUG(x) { x }
#endif

#if SK_RELEASE
	// TODO(moro): Add Notification for release
	#define SK_DEBUG_BREAK() __debugbreak()
	#define SK_ENABLE_MEMORY_TRACING 0
	#define SK_ENABLE_ASSERT 1
	#define SK_ENABLE_VERIFY 2
	#define SK_IF_DEBUG(...)
#endif

#if (!defined(SK_RELEASE) && !defined(SK_DEBUG)) || (defined(SK_RELEASE) && defined(SK_DEBUG))
#error Invalid Configuration
#endif

#define SK_ENABLE_PERF 1
#define SK_DISABLE_DEPRECATED 1

#define IMGUI_DEFINE_MATH_OPERATORS 1
#define SPDLOG_FMT_EXTERNAL 1

#define BIT(x) (1 << x)

#define SK_STRINGIFY(x) #x
#define SK_EXPAND(x) x
#define SK_CONNECT(a, b) a##b

#define SK_BIND_EVENT_FN(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }

#define SK_NOT_IMPLEMENTED() SK_CORE_ASSERT(false, "Not Implemented");

#if defined(SK_DISABLE_DEPRECATED) && (SK_DISABLE_DEPRECATED == 1)
#define SK_DEPRECATED(message)
#else
#define SK_DEPRECATED(message) [[deprecated(message)]]
#endif


#ifdef __COUNTER__
#define SK_UNIQUE_VAR_NAME SK_CONNECT(unique_var_, __COUNTER__)
#else
#define SK_UNIQUE_VAR_NAME SK_CONNECT(almoust_unique_var_, __LINE__)
#endif


#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
#define SK_FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define SK_FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
#define SK_FUNC_SIG __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define SK_FUNC_SIG __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define SK_FUNC_SIG __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
#define SK_FUNC_SIG __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
#define SK_FUNC_SIG __func__
#else
#define SK_FUNC_SIG "(unknown)"
#error No Function Signature
#endif

#include <stdint.h>

namespace Shark {

	using byte = unsigned char;

	struct Empty { template<typename T> Empty(const T&) {} };

	struct RenderID
	{
		uintptr_t ID;
		
		constexpr RenderID() : ID((uintptr_t)nullptr) {}
		constexpr RenderID(std::nullptr_t) : ID((uintptr_t)nullptr) {}
		constexpr RenderID(uintptr_t id) : ID(id) {}
		constexpr RenderID(void* id) : ID((uintptr_t)id) {}
		constexpr operator uintptr_t() const { return ID; }
		constexpr operator void*() const { return (void*)ID; }

		constexpr bool operator==(const RenderID& rhs) const { return ID == rhs.ID; }
		constexpr bool operator!=(const RenderID& rhs) const { return !(*this == rhs); }
		constexpr operator bool() const { return ID; }
	};

#if SK_PLATFORM_WINDOWS
	using WindowHandle = HWND;
#else
	using WindowHandle = void*;
#endif

	static_assert(sizeof(uint8_t) == 1);
	static_assert(sizeof(uint16_t) == 2);
	static_assert(sizeof(uint32_t) == 4);
	static_assert(sizeof(uint64_t) == 8);

	static_assert(sizeof(int8_t) == 1);
	static_assert(sizeof(int16_t) == 2);
	static_assert(sizeof(int32_t) == 4);
	static_assert(sizeof(int64_t) == 8);

	static_assert(sizeof(char) == 1);
	static_assert(sizeof(wchar_t) == 2);
	static_assert(sizeof(int) == 4);
	static_assert(sizeof(byte) == 1);

	static_assert(sizeof(float) == 4);
	static_assert(sizeof(double) == 8);

}


#include "Shark/Memory/Allocator.h"

#include "Shark/Core/Log.h"
#include "Shark/Core/Assert.h"

#include "Shark/Core/RefCount.h"
#include "Shark/Core/Scope.h"
