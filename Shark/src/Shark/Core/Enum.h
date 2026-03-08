#pragma once

#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_containers.hpp>

namespace Shark {

	namespace Concepts {

		template<typename T>
		concept EnumFlag = std::is_enum_v<T> && magic_enum::customize::enum_range<T>::is_flags;

		template<typename T>
		concept Enum = std::is_enum_v<T>;

	}

	namespace Enum {
		
		template<typename T>
		constexpr std::underlying_type_t<T> Underlying(T value) noexcept {
			return static_cast<std::underlying_type_t<T>>(value);
		}

		template<Concepts::EnumFlag TFlag>
		constexpr bool HasFlag(TFlag flags, TFlag flag)
		{
			return static_cast<TFlag>(Underlying(flags) & Underlying(flags)) == flag;
		}

		template<typename E, typename T>
		using Array = magic_enum::containers::array<E, T>;

	}

	template<Concepts::EnumFlag T>
	constexpr T operator~(T rhs)
	{
		return static_cast<T>(~Enum::Underlying(rhs));
	}

	template<Concepts::EnumFlag T>
	constexpr T operator|(T lhs, T rhs)
	{
		return static_cast<T>(Enum::Underlying(lhs) | Enum::Underlying(rhs));
	}

	template<Concepts::EnumFlag T>
	constexpr T operator&(T lhs, T rhs)
	{
		return static_cast<T>(Enum::Underlying(lhs) & Enum::Underlying(rhs));
	}

	template<Concepts::EnumFlag T>
	constexpr T operator^(T lhs, T rhs)
	{
		return static_cast<T>(Enum::Underlying(lhs) ^ Enum::Underlying(rhs));
	}

	template<Concepts::EnumFlag T>
	constexpr T& operator|=(T& lhs, T rhs)
	{
		return lhs = (lhs | rhs);
	}

	template<Concepts::EnumFlag T>
	constexpr T& operator&=(T& lhs, T rhs)
	{
		return lhs = (lhs & rhs);
	}

	template<Concepts::EnumFlag T>
	constexpr T& operator^=(T& lhs, T rhs)
	{
		return lhs = (lhs ^ rhs);
	}

}
