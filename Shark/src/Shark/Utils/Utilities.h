#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Concepts.h"

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

	template<bool TCond, typename T>
	using add_const_conditional_t = std::conditional_t<TCond, std::add_const_t<T>, T>;

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
		requires (std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<_Rng>, _Pj>, const _Ty*> &&
				  (specialization<std::ranges::range_value_t<_Rng>, Scope> || specialization<std::ranges::range_value_t<_Rng>, Ref>))
	[[nodiscard]] static constexpr std::add_pointer_t<typename std::ranges::range_value_t<_Rng>::value_type> find_as_ptr(_Rng&& _Range, const _Ty& _Val, _Pj _Proj = {})
	{
		auto i = std::ranges::find(std::forward<_Rng>(_Range), _Val, _Proj);
		if (i == std::ranges::end(_Range))
			return nullptr;

		return (*i).Raw();
	}

	template <std::ranges::input_range _Rng, class _Ty, class _Pj = std::identity, class _GetPj = std::identity>
		requires (
			std::indirect_binary_predicate<std::ranges::equal_to, std::projected<std::ranges::iterator_t<_Rng>, _Pj>, const _Ty*> &&
			specialization<std::invoke_result_t<_GetPj, std::ranges::range_value_t<_Rng>>, Ref>
		)
	[[nodiscard]] static constexpr std::remove_cvref_t<std::invoke_result_t<_GetPj, std::ranges::range_value_t<_Rng>>> find_as_ref(_Rng&& _Range, const _Ty& _Val, _Pj _Proj = {}, _GetPj _GetProj = {})
	{
		auto i = std::ranges::find(std::forward<_Rng>(_Range), _Val, _Proj);
		if (i == std::ranges::end(_Range))
			return nullptr;

		return std::invoke(_GetProj, (*i));
	}

}
