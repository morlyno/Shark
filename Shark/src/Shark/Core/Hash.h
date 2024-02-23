#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"

namespace Shark {

	class Hash
	{
	public:
		// FNV1a Hash Function
		static uint64_t GenerateFNV(const std::string& str);
		static uint64_t GenerateFNV(Buffer buffer);
	};

}
