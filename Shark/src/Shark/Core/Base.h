#pragma once

#include "PlatformDetection.h"

#ifdef SK_PLATFORM_WINDOWS
#define SK_DEBUG_BREAK() __debugbreak()
#endif

#define SK_ENABLE_ASSERT 1
#define SK_LOG_FILESYSTEM 1
#define IMGUI_DEFINE_MATH_OPERATORS

#define BIT(x) (1 << x)
#define SK_BIT(x) (1 << x)

#define SK_BIND_EVENT_FN(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }

#define SK_STRINGIFY(x) #x
#define SK_EXPAND(x) x

#define SK_LINE_VAR3(name, line) name##line
#define SK_VAR_NAME_CURR_LINE(name, line) SK_LINE_VAR3(name, line)

#ifdef SK_DEBUG
#define SK_IF_DEBUG(x) { x }
#define SK_DEBUG_RETURN_VAL(x) auto&& (x) =
#define SK_DEBUG_RETURN_TEMP SK_DEBUG_RETURN_VAL(SK_VAR_NAME_CURR_LINE(_TEMP_VAR_, __LINE__))
#else
#define SK_IF_DEBUG(...)
#define SK_DEBUG_RETURN_TEMP
#define SK_DEBUG_RETURN_VAL(x)
#endif


#include <stdint.h>

namespace Shark {

	using byte = unsigned char;

	struct Empty { template<typename T> Empty(const T&) {} };

	struct RenderID
	{
		uintptr_t ID;
		
		constexpr RenderID(std::nullptr_t) : ID((uintptr_t)nullptr) {}
		constexpr RenderID(uintptr_t id) : ID(id) {}
		constexpr RenderID(void* id) : ID((uintptr_t)id) {}
		operator uintptr_t() const { return ID; }
		operator void*() const { return (void*)ID; }

		bool operator==(const RenderID& rhs) const { return ID == rhs.ID; }
		bool operator!=(const RenderID& rhs) const { return !(*this == rhs); }
		operator bool() const { return ID; }

		template<typename T>
		T* As() { return (T*)ID; }
	};

	constexpr RenderID NullID = nullptr;

	using WindowHandle = void*;

}

#include "Shark/Core/Memory.h"
#include "Shark/Core/Allocator.h"

#include "Shark/Core/Log.h"
#include "Shark/Core/Assert.h"

#include "Shark/Core/RefCount.h"
#include "Shark/Core/Scope.h"
