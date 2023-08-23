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

		template<typename TKey, typename... TArgs>
		bool Contains(const std::map<TArgs...>& map, const TKey& key)
		{
			static_assert(std::is_same_v<std::map<TArgs...>::key_type, TKey>);
			return map.find(key) != map.end();
		}

	}

}
