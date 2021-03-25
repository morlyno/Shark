#pragma once

#include "PlatformDetection.h"

#ifdef SK_PLATFORM_WINDOWS
#define SK_DEBUG_BREAK() __debugbreak()
#endif

#define SK_ENABLE_ASSERT

#define SK_BIT(x) (1 << x)

#define SK_BIND_EVENT_FN(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }

#define SK_STRINGIFY(x) #x
#define SK_EXPAND(x) x

#include <memory>

namespace Shark {

	template<typename T>
	using Scope = std::unique_ptr<T>;

	template<typename T, typename... Args>
	constexpr Scope<T> CreateScope(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

}

#include "Shark/Core/Log.h"
#include "Shark/Core/Assert.h"
#include "Shark/Core/RefCount.h"