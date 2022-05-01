#pragma once

#include <memory>
#include <utility>
#include <functional>
#include <algorithm>
#include <math.h>
#include <stdlib.h>

#include <array>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <sstream>
#include <bitset>
#include <optional>
#include <queue>

#include <iostream>
#include <fstream>
#include <filesystem>

#include <chrono>

#include <fmt/format.h>

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#if SK_PLATFORM_WINDOWS
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <Windows.h>
#endif

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
