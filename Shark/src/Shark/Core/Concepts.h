#pragma once

#include "Shark/Core/Base.h"

#include <concepts>

namespace Shark::TypeTraits {

	namespace details {

		///////////////////////////////////////////////////////////////////////////
		//// member return type ///////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////

		template<typename T>
		struct member_return_type_object;

		template<typename Return, typename Object>
		struct member_return_type_object<Return Object::*>
		{
			using type = Return;
		};

		template<typename T>
		struct member_return_type_function;

		template<typename Return, typename Object, typename... Args>
		struct member_return_type_function<Return(Object::*)(Args...)>
		{
			using type = Return;
		};

		template<typename Return, typename Object, typename... Args>
		struct member_return_type_function<Return(Object::*)(Args...) const>
		{
			using type = Return;
		};

		///////////////////////////////////////////////////////////////////////////
		//// function args ////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////

		template<typename T>
		struct function_args;

		template<typename Return, typename... Args>
		struct function_args<Return(*)(Args...)>
		{
			using type = std::tuple<Args...>;
		};

		template<typename Return, typename Object, typename... Args>
		struct function_args<Return(Object::*)(Args...)>
		{
			using type = std::tuple<Args...>;
		};

		template<typename Return, typename Object, typename... Args>
		struct function_args<Return(Object::*)(Args...) const>
		{
			using type = std::tuple<Args...>;
		};

		template<typename T>
			requires requires { &T::operator(); }
		struct function_args<T> : function_args<decltype(&T::operator())>
		{
		};

	}

	///////////////////////////////////////////////////////////////////////////
	//// member return type ///////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	template<typename T>
		requires std::is_member_pointer_v<T>
	using member_return_type = typename
		std::conditional_t<
			std::is_member_object_pointer_v<T>,
			details::member_return_type_object<T>,
			details::member_return_type_function<T>
		>::type;

	///////////////////////////////////////////////////////////////////////////
	//// function args ////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	template<typename T>
	using function_args = details::function_args<T>;

	template<typename T>
	using function_args_type = typename details::function_args<T>::type;

	template <class T, template <class...> class Template>
	constexpr bool is_specialization_v = false;

	template <template <class...> class Template, class... Types>
	constexpr bool is_specialization_v<Template<Types...>, Template> = true;

	template<typename T, template<typename...> typename Template>
	struct is_specialization : std::bool_constant<is_specialization_v<T, Template>> {};

}

namespace Shark {

	template<typename B>
	concept boolean_testable = std::convertible_to<B, bool> && requires(B&& b)
	{
		{ !std::forward<B>(b) } -> std::convertible_to<bool>;
	};

	template<typename T, template<typename...> typename Template>
	concept specialization = requires { TypeTraits::is_specialization_v<T, Template>; };

}

namespace Shark::Tuple {

	namespace details {

		template<typename... Args, typename Func>
		void Each(Func&& func)
		{
			(func.template operator()<Args>(), ...);
		}
		
		template<typename... Args, size_t... Indices, typename Func>
		void EachIndexed(Func&& func, std::index_sequence<Indices...>)
		{
			(func.template operator()<Args, Indices>(), ...);
		}

	}

	template<typename... Args, typename Func>
	void Each(Func&& func)
	{
		details::Each<Args...>(std::forward<Func>(func));
	}

	template<template<typename...> typename Tuple, typename... Args, typename Func>
	void Each(Tuple<Args...>, Func&& func)
	{
		details::Each<Args...>(std::forward<Func>(func));
	}

	template<typename... Args, typename Func>
	void EachIndexed(Func&& func)
	{
		details::EachIndexed<Args...>(std::forward<Func>(func), std::index_sequence_for<Args...>{});
	}

	template<template<typename...> typename Tuple, typename... Args, typename Func>
	void EachIndexed(Tuple<Args...>, Func&& func)
	{
		details::EachIndexed<Args...>(std::forward<Func>(func), std::index_sequence_for<Args...>{});
	}

}
