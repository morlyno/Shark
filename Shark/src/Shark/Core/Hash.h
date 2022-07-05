#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	class Hash
	{
	public:
		// FNV1a Hash Function
		static uint64_t GenerateFNV(const std::string& str);
	};

}
