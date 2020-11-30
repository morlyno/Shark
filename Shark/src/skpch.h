#pragma once

#include "Shark/Core/PlatformDetection.h"

#ifdef SK_PLATFORM_WINDOWS
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
#endif

#include <memory>
#include <utility>
#include <functional>
#include <algorithm>
#include <math.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>


#ifdef SK_PLATFORM_WINDOWS
	#include <Windows.h>
	#include <d3d11.h>
	#include <d3dcompiler.h>
	#include <wrl.h>
	#include <DirectXMath.h>
	#include <DirectXColors.h>
#endif