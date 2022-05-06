#pragma once

#if SK_PLATFORM_WINDOWS

#include "Platform/Windows/WindowsUtils.h"

namespace Shark {
	using PlatformUtils = WindowsUtils;
}

#else
#error Invalid Platform
#endif
