#pragma once

#include "PlatformDetection.h"

#ifdef SK_PLATFORM_WINDOWS
#define SK_DEBUG_BREAK() __debugbreak()
#endif

// Always stops Application in Debug and Release
#define SK_CORE_STOP_APPLICATION(...) { SK_CORE_ERROR(__VA_ARGS__); __debugbreak(); } // TODO: Probably popup window / exeption

#define SK_ENABLE_ASSERT

#define SK_BIT(x) (1 << x)

#define SK_BIND_EVENT_FN(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }
#define SK_BIND_EVENT_FN_OF(func, src) [](auto&&... args) -> decltype(auto) { return (src)->func(std::forward<decltype(args)>(args)...); }

#define SK_TYPE_PUN(type,x) *reinterpret_cast<type*>(&x)

#define SK_STRINGIFY(x) #x
#define SK_EXPAND(x) x

//#define IM_ASSERT(_EXPR) SK_ASSERT(_EXPR)

#include <memory>

namespace Shark {

	template<typename T>
	using Scope = std::unique_ptr<T>;

	template<typename T, typename... Args>
	constexpr Scope<T> Create_Scope(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}
	
	template<typename T>
	using Ref = std::shared_ptr<T>;

	template<typename T, typename... Args>
	constexpr Ref<T> Create_Ref(Args&&... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}

#include "Shark/Core/Log.h"
#include "Shark/Core/Assert.h"