#pragma once

#if !SK_PLATFORM_WINDOWS
	#error Platform other than Windows are currently not supported
#endif

#if (!defined(SK_RELEASE) && !defined(SK_DEBUG)) || (defined(SK_RELEASE) && defined(SK_DEBUG))
#error Invalid Configuration
#endif


#if SK_DEBUG
	#define SK_DEBUG_BREAK() __debugbreak()
	#define SK_ENABLE_MEMORY_TRACING 0
	#define SK_ENABLE_ASSERT 1
	#define SK_ENABLE_VERIFY 1
	#define SK_IF_DEBUG(x) { x }
#endif

#if SK_RELEASE
	#define SK_DEBUG_BREAK() __debugbreak()
	#define SK_ENABLE_MEMORY_TRACING 0
	#define SK_ENABLE_ASSERT 1
	#define SK_ENABLE_VERIFY 2
	#define SK_IF_DEBUG(...)
#endif

#define SK_ENABLE_PERF 1

#define BIT(x) (1 << x)

#define SK_STRINGIFY(x) #x
#define SK_EXPAND(x) x
#define SK_CONNECT(a, b) a##b

#define SK_BIND_EVENT_FN(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }

#define SK_NOT_IMPLEMENTED() SK_CORE_ASSERT(false, "Not Implemented");
#define SK_DEPRECATED(message) [[deprecated(message)]]
#define SK_UNIQUE_VAR_NAME SK_CONNECT(unique_var_, __COUNTER__)

#define IM_VEC2_CLASS_EXTRA \
ImVec2(const glm::vec2& v) : ImVec2(v.x, v.y) {}\
explicit ImVec2(const glm::ivec2& v) : ImVec2((float)v.x, (float)v.y) {}\
explicit ImVec2(const glm::uvec2& v) : ImVec2((float)v.x, (float)v.y) {}\
operator glm::vec2() const { return { x, y }; }\
explicit operator glm::ivec2() const { return { (int)x, (int)y }; }\
explicit operator glm::uvec2() const { return { (uint32_t)x, (uint32_t)y }; }\

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

	using WindowHandle = void*;

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

	static_assert(std::is_same_v<std::filesystem::path::string_type::value_type, wchar_t>);

}


#include "Shark/Core/Log.h"
#include "Shark/Core/Assert.h"

#include "Shark/Core/RefCount.h"
#include "Shark/Core/Scope.h"
