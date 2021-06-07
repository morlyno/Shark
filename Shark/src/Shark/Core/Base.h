#pragma once

#include "PlatformDetection.h"

#ifdef SK_PLATFORM_WINDOWS
#define SK_DEBUG_BREAK() __debugbreak()
#endif

#define SK_ENABLE_ASSERT 1

#define BIT(x) (1 << x)
#define SK_BIT(x) (1 << x)

#define SK_BIND_EVENT_FN(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }

#define SK_STRINGIFY(x) #x
#define SK_EXPAND(x) x

#ifdef SK_DEBUG
#define SK_IF_DEBUG(x) { x }
#else
#define SK_IF_DEBUG(...)
#endif

#include <stdint.h>

namespace Shark {

	using byte = unsigned char;

	struct Empty { template<typename T> Empty(const T&) {} };
	 
	struct RenderID
	{
		uintptr_t ID;
		
		RenderID(uintptr_t id) : ID(id) {}
		RenderID(void* id) : ID((uintptr_t)id) {}
		operator uintptr_t() const { return ID; }
		operator void*() const { return (void*)ID; }

		template<typename T>
		T* As() { return (T*)ID; }
	};

	using WindowHandle = void*;

}

#include "Shark/Core/Memory.h"
#include "Shark/Core/Allocator.h"

#include "Shark/Core/Log.h"
#include "Shark/Core/Assert.h"

#include "Shark/Core/RefCount.h"
#include "Shark/Core/Scope.h"
