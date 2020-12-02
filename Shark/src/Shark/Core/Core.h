#pragma once

#include "PlatformDetection.h"

#define SK_BIT(x) (1 << x)

#define SK_BIND_EVENT_FN(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }
#define SK_BIND_EVENT_FN_OF(func, src) [](auto&&... args) -> decltype(auto) { return (src)->func(std::forward<decltype(args)>(args)...); }

#ifdef SK_DEBUG
	#define SK_ENABLE_ASSERT
#endif

#ifdef SK_ENABLE_ASSERT
	#ifdef SK_PLATFORM_WINDOWS
		#define SK_ASSERT(x) if(!(x)) __debugbreak()
	#endif
#else
	#define SK_ASSERT(...)
#endif

#ifdef SK_DEBUG
#define SK_BREAK_IF(x) if ((x)) __debugbreak()
#define SK_BREAK()  __debugbreak()
#else
#define SK_BREAK_IF(x)
#define SK_BREAK(x)
#endif

#define SK_TYPE_PUN(type,x) *reinterpret_cast<type*>(&x)

#define SK_STRINGIFY(x) #x