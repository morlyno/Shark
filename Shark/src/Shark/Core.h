#pragma once

/*-----Temporary in this file-----*/
#ifdef _WIN32
	#ifdef _WIN64
		#define SK_PLATFORM_WINDOWS
	#else
		#error x86 Builds not supported
	#endif
#elif #defined(__APPLE__)
	#error Appel not supported
#elif #defined(__linux__)
	#error Linux not supported
#elif #defined(__ANDROID__)
	#error Android not supported
#else
	#error Platform not detected
#endif
/*--------------------------------*/

#ifdef SK_PLATFORM_WINDOWS
	#ifdef SK_BUILD_DLL
		#define SHARK_API __declspec(dllexport)
	#else
		#define SHARK_API __declspec(dllimport)
	#endif
#else
	#error Shark only supports Windows
#endif