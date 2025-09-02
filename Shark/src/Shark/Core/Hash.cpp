#include "skpch.h"
#include "Hash.h"

namespace Shark {

	void Hash::AppendFNV(uint64_t& seed, uint64_t value)
	{
		seed ^= value;
		seed *= FNVPrime;
	}

	void Hash::AppendFNV(uint64_t& seed, const Buffer values)
	{
		uint8_t* first = values.As<uint8_t>();
		for (uint64_t i = 0; i < values.Size; i++)
		{
			seed ^= first[i];
			seed *= FNVPrime;
		}
	}

	uint64_t Hash::CombineFNV(uint64_t seed, uint64_t value)
	{
		AppendFNV(seed, value);
		return seed;
	}

	uint64_t Hash::CombineFNV(uint64_t seed, const Buffer values)
	{
		AppendFNV(seed, values);
		return seed;
	}

	uint64_t Hash::GenerateFNV(const std::string_view str)
	{
		return CombineFNV(FNVBase, { (void*)str.data(), str.size() });
	}

	uint64_t Hash::GenerateFNV(const Buffer buffer)
	{
		return CombineFNV(FNVBase, buffer);
	}

	void Hash::HashCombine(uint64_t& seed, uint64_t hash)
	{
		hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= hash;
	}

}
