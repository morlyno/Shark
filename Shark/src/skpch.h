#pragma once

#include "Shark/Core/PlatformDetection.h"

#include <memory>
#include <utility>
#include <functional>
#include <algorithm>
#include <math.h>

#include <array>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <bitset>
#include <optional>

#include <iostream>
#include <fstream>
#include <filesystem>

#include <chrono>

#ifdef SK_PLATFORM_WINDOWS
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <Windows.h>
#endif

#include "Shark/Core/Base.h"
