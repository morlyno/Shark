#include "skpch.h"
#include "Hash.h"

namespace Shark {

	static constexpr uint64_t s_FNV_Offset_Basis = 14695981039346656037;
	static constexpr uint64_t s_FNV_Prime = 1099511628211;

	uint64_t FNV1A_AppendBytes(uint64_t base, const uint8_t* data, uint64_t byteSize)
	{
		for (uint64_t i = 0; i < byteSize; i++)
		{
			base ^= (uint64_t)data[i];
			base *= s_FNV_Prime;
		}
		return base;
	}

	template<typename T>
	uint64_t FNV1A_AppendValue(uint64_t base, const T& val)
	{
		static_assert(std::is_trivial_v<T>);
		return FNV1A_AppendBytes(base, (const uint8_t*)&val, sizeof(T));
	}

	uint64_t Hash::FNV1A(const std::string& str)
	{
		return FNV1A_AppendBytes(s_FNV_Offset_Basis, (const uint8_t*)str.data(), str.size());
	}

	uint64_t Hash::FNV1A(uint64_t val)
	{
		return FNV1A_AppendValue(s_FNV_Offset_Basis, val);
	}

	uint64_t Hash::FNV1A(const void* ptr)
	{
		return FNV1A_AppendValue(s_FNV_Offset_Basis, ptr);
	}

}
