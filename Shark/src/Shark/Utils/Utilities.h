#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	namespace Projection {

		struct ProjectionToAddress
		{
			template<typename T>
			T* operator()(T& val)
			{
				return &val;
			}
		};

		static constexpr ProjectionToAddress ToAddress;

	}

}
