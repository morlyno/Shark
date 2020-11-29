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

#define SK_BIT(x) (1 << x)

#ifdef SK_ENABLE_ASSERT
	#define SK_ASSERT(x) if(!(x)) __debugbreak()
#else
	#define SK_ASSERT(...)
#endif

#ifdef SK_DEBUG
#define SK_BREAK_IF(x) if ((x)) __debugbreak()
#define SK_BREAK()  __debugbreak()
#else
#define SK_BREAK_IF(x)
#define SK_BREAK(x)
#endif

#define SK_TYPE_PUN(type,x) *reinterpret_cast<type*>(&x)

namespace Shark {
	struct SHARK_API F32RGBA
	{
		union
		{
			struct { float r,g,b,a; };
			float rgba[4];
		};
		F32RGBA( float r,float g,float b,float a = 1.0f )
			: r( r ),g( g ),b( b ),a( a )
		{}
	};
}

#ifdef SK_PLATFORM_WINDOWS
	#define SK_WINDOW_HANDLE HWND
#else
#error Shark only supports Windows
#endif