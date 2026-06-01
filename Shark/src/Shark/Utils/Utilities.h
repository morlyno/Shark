#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	namespace Projection {

		struct ProjectionToAddress
		{
			template<typename T>
			T* operator()(T& val)
			{
				return &val;
			}
		};

		static constexpr ProjectionToAddress ToAddress;

	}

	template<std::ranges::range TRange>
	bool Contains(const TRange& range, const typename TRange::value_type& value)
	{
		return std::ranges::find(range, value) != std::ranges::end(range);
	}

}
