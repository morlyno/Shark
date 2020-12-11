#pragma once

#include "PlatformDetection.h"

#ifdef SK_PLATFORM_WINDOWS
#define SK_DEBUG_BREAK __debugbreak()
#endif

#define SK_ENABLE_ASSERT

#define SK_BIT(x) (1 << x)

#define SK_BIND_EVENT_FN(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }
#define SK_BIND_EVENT_FN_OF(func, src) [](auto&&... args) -> decltype(auto) { return (src)->func(std::forward<decltype(args)>(args)...); }

#define SK_TYPE_PUN(type,x) *reinterpret_cast<type*>(&x)

#define SK_STRINGIFY(x) #x
#define SK_EXPAND(x) x

//#define IM_ASSERT(_EXPR) SK_ASSERT(_EXPR)

#include "Shark/Core/Log.h"
#include "Shark/Core/Assert.h"
