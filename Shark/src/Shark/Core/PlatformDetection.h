#pragma once

#ifdef _WIN32
	#ifdef _WIN64
		#define	SK_PLATFORM_WINDOWS
	#else
		#error Shack does not support x86 builds
	#endif
#elif #defined(__APPLE__) || defined(__MACH__)
	#include <TargetConditionals.h>
	#if TARGET_IPHONE_SIMULATOR == 1
		#error IOS simulator is not supported!
	#elif TARGET_OS_IPHONE == 1
		#define HZ_PLATFORM_IOS
		#error IOS is not supported!
	#elif TARGET_OS_MAC == 1
		#define HZ_PLATFORM_MACOS
		#error MacOS is not supported!
	#else
		#error Unknown Apple platform!
	#endif
#elif #defined(__ANDROID__)
	#define SK_PLATFORM_ANDROID
	#error Android is not supportet
#elif #defined(__linux__)
	#define SK_PLATFORM_LINUX
	#error Linux is not supportet
#else
	#error Unknown Platform
#endif