#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class Hash
	{
	public:
		static uint64_t FNV1A(const std::string& str);
		static uint64_t FNV1A(uint64_t val);
		static uint64_t FNV1A(const void* ptr);
	};

}
