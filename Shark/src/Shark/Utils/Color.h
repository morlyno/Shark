#pragma once

#include "Shark/Core/Core.h"

namespace Shark {

	namespace Color {
		template<typename T>
		struct T_RGBA
		{
			union
			{
				struct { T r,g,b,a; };
				T rgba[4];
			};
			T_RGBA( T r,T g,T b,T a = (T)1.0 )
				: r( r ),g( g ),b( b ),a( a )
			{}
		};

		typedef T_RGBA<float> F32RGBA;
		typedef T_RGBA<double> D64RGBA;
		typedef T_RGBA<unsigned char> C8RGBA;
	}

}