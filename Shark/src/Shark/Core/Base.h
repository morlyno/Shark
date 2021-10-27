#pragma once

#if SK_PLATFORM_WINDOWS
	#define SK_DEBUG_BREAK() __debugbreak()
#else
	#error Platform other than Windows are currently not supported
#endif

#if SK_DEBUG
	#define SK_ENABLE_MEMORY_TRACING 0
	#define SK_ENABLE_ASSERT 1
	#define SK_ENABLE_VERIFY 1
	#define SK_IF_DEBUG(x) { x }
#endif

#if SK_RELEASE
	#define SK_ENABLE_MEMORY_TRACING 0
	#define SK_ENABLE_ASSERT 1
	#define SK_ENABLE_VERIFY 2
	#define SK_IF_DEBUG(...)
#endif


#define IMGUI_DEFINE_MATH_OPERATORS 1
#define SPDLOG_FMT_EXTERNAL 1

#define BIT(x) (1 << x)
#define SK_BIT(x) (1 << x)

#define SK_STRINGIFY(x) #x
#define SK_EXPAND(x) x
#define SK_CONNECT(a, b) a##b

#define SK_BIND_EVENT_FN(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }

#if defined(SK_DISABLE_DEPRECATED) && (SK_DISABLE_DEPRECATED == 1) || 1
#define SK_DEPRECATED(message)
#else
#define SK_DEPRECATED(message) [[deprecated(message)]]
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

}

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
static_assert(sizeof(Shark::byte) == 1);

#include "Shark/Memory/Allocator.h"

#include "Shark/Core/Log.h"
#include "Shark/Core/Assert.h"

#include "Shark/Core/RefCount.h"
#include "Shark/Core/Scope.h"
