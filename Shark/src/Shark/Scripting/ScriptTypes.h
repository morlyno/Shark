#pragma once

#include "Shark/Core/Base.h"

extern "C" {
	typedef struct _MonoAssembly MonoAssembly;
	typedef struct _MonoImage MonoImage;
}

namespace Shark {

	using GCHandle = uint32_t;

	struct AssemblyInfo
	{
		MonoAssembly* Assembly;
		MonoImage* Image;
		std::filesystem::path FilePath;
	};

}
