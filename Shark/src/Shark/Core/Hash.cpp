#include "skpch.h"
#include "Hash.h"

namespace Shark {

	static constexpr uint64_t s_FNV_Offset_Basis = 14695981039346656037;
	static constexpr uint64_t s_FNV_Prime = 1099511628211;

	uint64_t Hash::GenerateFNV(const std::string_view str)
	{
		uint64_t hash = s_FNV_Offset_Basis;
		for (auto& c : str)
		{
			hash ^= (uint64_t)c;
			hash *= s_FNV_Prime;
		}
		return hash;
	}

	uint64_t Hash::GenerateFNV(Buffer buffer)
	{
		uint64_t hash = s_FNV_Offset_Basis;
		uint8_t* ptr = buffer.As<uint8_t>();
		while (ptr != buffer.End())
		{
			hash ^= (uint64_t)*ptr++;
			hash *= s_FNV_Prime;
		}
		return hash;
	}

}
