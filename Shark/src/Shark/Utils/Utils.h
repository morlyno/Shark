#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	namespace Utils {

		template<typename T>
		constexpr T* ToPointer(T& val)
		{
			return &val;
		}
		
		template<typename T>
		constexpr T* ToPointer(T* val)
		{
			return val;
		}

		std::string BytesToString(uint64_t bytes);

	}

}
