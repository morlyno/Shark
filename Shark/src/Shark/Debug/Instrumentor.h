#pragma once

#include <optick.h>

#define SK_PROFILE_FRAME(name, ...) OPTICK_FRAME(name, __VA_ARGS__)
#define SK_PROFILE_THREAD(name) OPTICK_THREAD(name)
#define SK_PROFILE_FUNCTION() OPTICK_EVENT()
#define SK_PROFILE_SCOPED(name) OPTICK_EVENT(name)
#define SK_PROFILE_SHUTDOWN() OPTICK_SHUTDOWN()
