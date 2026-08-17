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

	template <class T, template <class...> class Template>
	constexpr bool is_specialization_v = false;

	template <template <class...> class Template, class... Types>
	constexpr bool is_specialization_v<Template<Types...>, Template> = true;

	template<typename T, template<typename...> typename Template>
	struct is_specialization : std::bool_constant<is_specialization_v<T, Template>> {};

	template<typename T, template<typename...> typename Template>
	concept specialization = requires { is_specialization_v<T, Template>; };

	template <std::ranges::input_range _Rng, class _Ty, class _Pj = std::identity>
		requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<_Rng>, _Pj>, const _Ty*>
	[[nodiscard]] static constexpr std::add_pointer_t<std::ranges::range_value_t<_Rng>> find_as_ptr(_Rng&& _Range, const _Ty& _Val, _Pj _Proj = {})
	{
		auto i = std::ranges::find(std::forward<_Rng>(_Range), _Val, _Proj);
		if (i == std::ranges::end(_Range))
			return nullptr;
		
		return &(*i);
	}

	template <std::ranges::input_range _Rng, class _Ty, class _Pj = std::identity>
		requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<_Rng>, _Pj>, const _Ty*> &&
				 (specialization<std::ranges::range_value_t<_Rng>, Scope> || specialization<std::ranges::range_value_t<_Rng>, Ref>)
	[[nodiscard]] static constexpr std::add_pointer_t<typename std::ranges::range_value_t<_Rng>::value_type> find_as_ptr(_Rng&& _Range, const _Ty& _Val, _Pj _Proj = {})
	{
		auto i = std::ranges::find(std::forward<_Rng>(_Range), _Val, _Proj);
		if (i == std::ranges::end(_Range))
			return nullptr;

		return (*i).Raw();
	}

	template<typename T, typename TFunc>
	auto and_then(const std::optional<T>& opt, TFunc&& func) -> std::optional<std::invoke_result_t<TFunc, T>>
	{
		if (opt)
			return std::optional(func(*opt));
		return {};
	}

}
