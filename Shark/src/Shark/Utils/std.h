#pragma once

#include <ranges>

namespace std {
	_EXPORT_STD struct from_range_t {
		explicit from_range_t() = default;
	};
	_EXPORT_STD inline constexpr from_range_t from_range;

	template <class _Rng, class _Elem>
	concept _Container_compatible_range =
		(_RANGES input_range<_Rng>) && convertible_to<_RANGES range_reference_t<_Rng>, _Elem>;

	template <_RANGES input_range _Rng>
	using _Range_key_type = remove_const_t<typename _RANGES range_value_t<_Rng>::first_type>;

	template <_RANGES input_range _Rng>
	using _Range_mapped_type = _RANGES range_value_t<_Rng>::second_type;

	template <_RANGES input_range _Rng>
	using _Range_to_alloc_type =
		pair<const typename _RANGES range_value_t<_Rng>::first_type, typename _RANGES range_value_t<_Rng>::second_type>;
}

namespace std::ranges {
	template <class _Range, class _Container>
	concept _Sized_and_reservable = sized_range<_Range> && sized_range<_Container>
		&& requires(_Container & _Cont, const range_size_t<_Container> _Count) {
		_Cont.reserve(_Count);
		{ _Cont.capacity() } -> same_as<range_size_t<_Container>>;
		{ _Cont.max_size() } -> same_as<range_size_t<_Container>>;
	};

	template <class _Rng, class _Container>
	concept _Ref_converts =
		!input_range<_Container> || convertible_to<range_reference_t<_Rng>, range_value_t<_Container>>;

	template <class _Rng, class _Container, class... _Types>
	concept _Common_constructible =
		common_range<_Rng> //
		&& requires { typename iterator_traits<iterator_t<_Rng>>::iterator_category; }
	&& derived_from<typename iterator_traits<iterator_t<_Rng>>::iterator_category, input_iterator_tag>
		&& constructible_from<_Container, iterator_t<_Rng>, iterator_t<_Rng>, _Types...>;

	template <class _Container, class _Reference>
	concept _Can_emplace_back = requires(_Container & _Cont) { _Cont.emplace_back(_STD declval<_Reference>()); };

	template <class _Container, class _Reference>
	concept _Can_push_back = requires(_Container & _Cont) { _Cont.push_back(_STD declval<_Reference>()); };

	template <class _Container, class _Reference>
	concept _Can_emplace_end = requires(_Container & _Cont) { _Cont.emplace(_Cont.end(), _STD declval<_Reference>()); };

	template <class _Container, class _Reference>
	concept _Can_insert_end = requires(_Container & _Cont) { _Cont.insert(_Cont.end(), _STD declval<_Reference>()); };

	template <class _Rng, class _Container, class... _Types>
	concept _Constructible_appendable = constructible_from<_Container, _Types...>
		&& (_Can_emplace_back<_Container, range_reference_t<_Rng>>
			|| _Can_push_back<_Container, range_reference_t<_Rng>>
			|| _Can_emplace_end<_Container, range_reference_t<_Rng>>
			|| _Can_insert_end<_Container, range_reference_t<_Rng>>);

	_EXPORT_STD template <class _Container, input_range _Rng, class... _Types>
		requires (!view<_Container>)
	_NODISCARD constexpr _Container to(_Rng&& _Range, _Types&&... _Args) {
		static_assert(!is_const_v<_Container>, "C must not be const. ([range.utility.conv.to])");
		static_assert(!is_volatile_v<_Container>, "C must not be volatile. ([range.utility.conv.to])");
		static_assert(is_class_v<_Container>, "C must be a class type. ([range.utility.conv.to])");
		if constexpr (_Ref_converts<_Rng, _Container>) {
			if constexpr (constructible_from<_Container, _Rng, _Types...>) {
				return _Container(_STD forward<_Rng>(_Range), _STD forward<_Types>(_Args)...);
			}
			else if constexpr (constructible_from<_Container, const from_range_t&, _Rng, _Types...>) { // per LWG-3845
				return _Container(from_range, _STD forward<_Rng>(_Range), _STD forward<_Types>(_Args)...);
			}
			else if constexpr (_Common_constructible<_Rng, _Container, _Types...>) {
				return _Container(_RANGES begin(_Range), _RANGES end(_Range), _STD forward<_Types>(_Args)...);
			}
			else if constexpr (_Constructible_appendable<_Rng, _Container, _Types...>) {
				_Container _Cont(_STD forward<_Types>(_Args)...);
				if constexpr (_Sized_and_reservable<_Rng, _Container>) {
					_Cont.reserve(static_cast<range_size_t<_Container>>(_RANGES size(_Range)));
				}
				for (auto&& _Elem : _Range) {
					using _ElemTy = decltype(_Elem);
					if constexpr (_Can_emplace_back<_Container, _ElemTy>) {
						_Cont.emplace_back(_STD forward<_ElemTy>(_Elem));
					}
					else if constexpr (_Can_push_back<_Container, _ElemTy>) {
						_Cont.push_back(_STD forward<_ElemTy>(_Elem));
					}
					else if constexpr (_Can_emplace_end<_Container, _ElemTy>) {
						_Cont.emplace(_Cont.end(), _STD forward<_ElemTy>(_Elem));
					}
					else {
						_STL_INTERNAL_STATIC_ASSERT(_Can_insert_end<_Container, _ElemTy>);
						_Cont.insert(_Cont.end(), _STD forward<_ElemTy>(_Elem));
					}
				}
				return _Cont;
			}
			else {
				static_assert(false, "ranges::to requires the result to be constructible from the source range, either "
							  "by using a suitable constructor, or by inserting each element of the range into "
							  "the default-constructed object. (N4981 [range.utility.conv.to]/2.1.5)");
			}
		}
		else if constexpr (input_range<range_reference_t<_Rng>>) {
			const auto _Xform = [](auto&& _Elem) _STATIC_CALL_OPERATOR{
				return _RANGES to<range_value_t<_Container>>(_STD forward<decltype(_Elem)>(_Elem));
			};
			return _RANGES to<_Container>(views::transform(ref_view{ _Range }, _Xform), _STD forward<_Types>(_Args)...);
		}
		else {
			static_assert(false,
						  "ranges::to requires the elements of the source range to be either implicitly convertible to the "
						  "elements of the destination container, or be ranges themselves for ranges::to to be applied "
						  "recursively. (N4981 [range.utility.conv.to]/2.3)");
		}
	}

	template <class _Container>
	struct _To_class_fn {
		_STL_INTERNAL_STATIC_ASSERT(!is_const_v<_Container>);
		_STL_INTERNAL_STATIC_ASSERT(!is_volatile_v<_Container>);
		_STL_INTERNAL_STATIC_ASSERT(is_class_v<_Container>);
		_STL_INTERNAL_STATIC_ASSERT(!view<_Container>);

		template <input_range _Rng, class... _Types>
		_NODISCARD _STATIC_CALL_OPERATOR constexpr auto operator()(
			_Rng&& _Range, _Types&&... _Args) _CONST_CALL_OPERATOR
			requires requires { _RANGES to<_Container>(_STD forward<_Rng>(_Range), _STD forward<_Types>(_Args)...); }
		{
			return _RANGES to<_Container>(_STD forward<_Rng>(_Range), _STD forward<_Types>(_Args)...);
		}
	};

	_EXPORT_STD template <class _Container, class... _Types>
		requires (!view<_Container>)
	_NODISCARD constexpr auto to(_Types&&... _Args) {
		static_assert(!is_const_v<_Container>, "C must not be const. ([range.utility.conv.adaptors])");
		static_assert(!is_volatile_v<_Container>, "C must not be volatile. ([range.utility.conv.adaptors])");
		static_assert(is_class_v<_Container>, "C must be a class type. ([range.utility.conv.adaptors])");
		return _Range_closure<_To_class_fn<_Container>, decay_t<_Types>...>{_STD forward<_Types>(_Args)...};
	}

	template <input_range _Rng>
	struct _Phony_input_iterator {
		using iterator_category = input_iterator_tag;
		using value_type = range_value_t<_Rng>;
		using difference_type = ptrdiff_t;
		using pointer = add_pointer_t<range_reference_t<_Rng>>;
		using reference = range_reference_t<_Rng>;

		reference operator*() const;
		pointer operator->() const;

		_Phony_input_iterator& operator++();
		_Phony_input_iterator operator++(int);

		bool operator==(const _Phony_input_iterator&) const;
	};

	template <template <class...> class _Cnt, class _Rng, class... _Args>
	auto _To_helper() {
		if constexpr (requires { _Cnt(_STD declval<_Rng>(), _STD declval<_Args>()...); }) {
			return static_cast<decltype(_Cnt(_STD declval<_Rng>(), _STD declval<_Args>()...))*>(nullptr);
		}
		else if constexpr (requires { _Cnt(from_range, _STD declval<_Rng>(), _STD declval<_Args>()...); }) {
			return static_cast<decltype(_Cnt(from_range, _STD declval<_Rng>(), _STD declval<_Args>()...))*>(nullptr);
		}
		else if constexpr (requires {
			_Cnt(_STD declval<_Phony_input_iterator<_Rng>>(),
				 _STD declval<_Phony_input_iterator<_Rng>>(), _STD declval<_Args>()...);
		}) {
			return static_cast<decltype(_Cnt(_STD declval<_Phony_input_iterator<_Rng>>(),
											 _STD declval<_Phony_input_iterator<_Rng>>(), _STD declval<_Args>()...))*>(nullptr);
		}
	}

	_EXPORT_STD template <template <class...> class _Container, input_range _Rng, class... _Types,
		class _Deduced = remove_pointer_t<decltype(_To_helper<_Container, _Rng, _Types...>())>>
		_NODISCARD constexpr _Deduced to(_Rng&& _Range, _Types&&... _Args) {
		return _RANGES to<_Deduced>(_STD forward<_Rng>(_Range), _STD forward<_Types>(_Args)...);
	}

	template <template <class...> class _Container>
	struct _To_template_fn {
		template <input_range _Rng, class... _Types,
			class _Deduced = remove_pointer_t<decltype(_To_helper<_Container, _Rng, _Types...>())>>
			_NODISCARD _STATIC_CALL_OPERATOR constexpr auto operator()(
				_Rng&& _Range, _Types&&... _Args) _CONST_CALL_OPERATOR {
			return _RANGES to<_Deduced>(_STD forward<_Rng>(_Range), _STD forward<_Types>(_Args)...);
		}
	};

	_EXPORT_STD template <template <class...> class _Container, class... _Types>
		_NODISCARD constexpr auto to(_Types&&... _Args) {
		return _Range_closure<_To_template_fn<_Container>, decay_t<_Types>...>{_STD forward<_Types>(_Args)...};
	}
}
