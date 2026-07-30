#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	namespace Projection {

		namespace Type {

			struct ToAddress
			{
				template<typename T>
				T* operator()(T& val)
				{
					return &val;
				}
			};

			struct Invoke
			{
				template<typename TFunc, typename... TArgs>
				auto operator()(TFunc&& func, TArgs&&... args)
				{
					return std::invoke(std::forward<TFunc>(func), std::forward<TArgs>(args)...);
				}
			};

		}

		static constexpr Type::ToAddress ToAddress;
		static constexpr Type::Invoke Invoke;
	}

	template<std::ranges::range TRange>
	bool Contains(const TRange& range, const typename TRange::value_type& value)
	{
		return std::ranges::find(range, value) != std::ranges::end(range);
	}

	template<typename T>
	bool ReserveAtLeast(std::vector<T>& vec, size_t requestedCapacity)
	{
		if (vec.capacity() < requestedCapacity)
		{
			size_t newCapacity = vec.capacity() + vec.capacity() / 2;
			if (newCapacity < requestedCapacity)
				newCapacity = requestedCapacity;

			vec.reserve(newCapacity);
		}
	}

	template<typename T, typename... TArgs>
	T PostIncement(T& t)
	{
		T proxy(t);
		++t;
		return proxy;
	}

	template<typename... TArgs>
		//requires (std::is_same_v<std::add_const_t<std::tuple_element_t<0, std::tuple<TArgs...>>>, std::add_const<TArgs>> && ...)
	std::array<std::tuple_element_t<0, std::tuple<TArgs...>>*, sizeof...(TArgs)> AsReferenceRange(TArgs&... args)
	{
		return { &args... };
	}

	template<typename String>
	concept StringLike = requires(String string)
	{
		{ std::string_view{ string } } -> std::same_as<std::string_view>;
	};

}
