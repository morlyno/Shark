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
		static void AppendFNV(uint64_t& seed, const Buffer values);

		static uint64_t CombineFNV(uint64_t seed, uint64_t value);
		static uint64_t CombineFNV(uint64_t seed, const Buffer values);

		static uint64_t GenerateFNV(const std::string_view str);
		static uint64_t GenerateFNV(const Buffer buffer);

		static void HashCombine(uint64_t& seed, uint64_t hash);

		template<typename TValue>
			requires std::is_scalar_v<TValue>
		static uint64_t GenerateFNV(const TValue& value)
		{
			return GenerateFNV(Buffer::FromValue<TValue>(value));
		}
	};

}
