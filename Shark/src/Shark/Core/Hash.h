#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"

namespace Shark {

	class Hash
	{
	public:
		static constexpr uint64_t FNVPrime = 1099511628211;
		static constexpr uint64_t FNVBase = 14695981039346656037;

		// FNV1a Hash Function
		static void AppendFNV(uint64_t& seed, uint64_t value);
		static void AppendFNV(uint64_t& seed, const void* values, uint64_t byteSize);
		static void AppendFNV(uint64_t& seed, const Buffer values);

		static uint64_t CombineFNV(uint64_t seed, uint64_t value);
		static uint64_t CombineFNV(uint64_t seed, const Buffer values);

		static uint64_t GenerateFNV(const Buffer buffer);

		static void HashCombine(uint64_t& seed, uint64_t hash);

		static constexpr uint64_t ConstexprHash(std::string_view str)
		{
			uint64_t seed = FNVBase;
			const char* first = str.data();
			for (uint64_t i = 0; i < str.length(); i++)
			{
				seed ^= first[i];
				seed *= FNVPrime;
			}
			return seed;
		}

	};

	template<typename T>
	uint64_t StandartHash(const T& value) { return std::hash<T>{}(value); }

	struct Hasher
	{
		template<typename T>
		uint64_t operator()(const T& val)
		{
			return StandartHash(val);
		}

		using is_transparent = int;
	};

}
