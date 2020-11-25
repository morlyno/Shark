#pragma once

#ifdef SK_PLATFORM_WINDOWS
	#ifdef SK_BUILD_DLL
		#define SHARK_API __declspec(dllexport)
	#else
		#define SHARK_API __declspec(dllimport)
	#endif
#else
	#error Shark only supports Windows
#endif

#define BIT(x) (1 << x)

#ifdef SK_ENABLE_ASSERT
	#define SK_ASSERT(x) if(!(x)) __debugbreak()
#else
	#define SK_ASSERT(...)
#endif