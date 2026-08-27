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
			return static_cast<TFlag>(Underlying(flags) & Underlying(flag)) == flag;
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

	namespace Enum {

		template<Concepts::Enum T>
		class Flags
		{
		public:
			constexpr Flags() = default;
			constexpr Flags(T flag) : m_Flags(flag) {}

			constexpr bool Any() const;
			constexpr bool Has(T flag) const;
			constexpr Flags& Set(T flag);
			constexpr Flags& Set(T flag, bool set);

			constexpr T Enum() const { return m_Flags; }
			constexpr std::underlying_type_t<T> Underlying() const { return Enum::Underlying(m_Flags); }

			constexpr operator T() const { return m_Flags; }

		private:
			T m_Flags;
		};

		template<typename T>
		constexpr T format_as(Flags<T> flags)
		{
			return flags.Enum();
		}

		template<Concepts::Enum T>
		constexpr bool operator==(Flags<T> lhs, Flags<T> rhs)
		{
			return lhs.Underlying() == rhs.Underlying();
		}

		template<Concepts::Enum T>
		constexpr bool operator==(Flags<T> lhs, T rhs)
		{
			return lhs == Flags(rhs);
		}

		template<Concepts::Enum T>
		constexpr bool operator==(T lhs, Flags<T> rhs)
		{
			return Flags(lhs) == rhs;
		}

		template<Concepts::Enum T>
		constexpr bool operator!=(Flags<T> lhs, Flags<T> rhs)
		{
			return lhs.Underlying() != rhs.Underlying();
		}

		template<Concepts::Enum T>
		constexpr bool operator!=(Flags<T> lhs, T rhs)
		{
			return lhs != Flags(rhs);
		}

		template<Concepts::Enum T>
		constexpr bool operator!=(T lhs, Flags<T> rhs)
		{
			return Flags(lhs) != rhs;
		}

		template<Concepts::Enum T>
		constexpr Flags<T> operator|(Flags<T> lhs, Flags<T> rhs)
		{
			return static_cast<T>(lhs.Underlying() | rhs.Underlying());
		}

		template<Concepts::Enum T>
		constexpr Flags<T> operator|(Flags<T> lhs, T rhs)
		{
			return lhs | Flags(rhs);
		}

		template<Concepts::Enum T>
		constexpr Flags<T> operator|(T lhs, Flags<T> rhs)
		{
			return Flags(lhs) | rhs;
		}

		template<Concepts::Enum T>
		constexpr Flags<T> operator&(Flags<T> lhs, Flags<T> rhs)
		{
			return static_cast<T>(lhs.Underlying() & rhs.Underlying());
		}

		template<Concepts::Enum T>
		constexpr Flags<T> operator&(Flags<T> lhs, T rhs)
		{
			return lhs & Flags(rhs);
		}

		template<Concepts::Enum T>
		constexpr Flags<T> operator&(T lhs, Flags<T> rhs)
		{
			return Flags(lhs) & rhs;
		}

		template<Concepts::Enum T>
		constexpr Flags<T> operator^(Flags<T> lhs, Flags<T> rhs)
		{
			return static_cast<T>(lhs.Underlying() ^ rhs.Underlying());
		}

		template<Concepts::Enum T>
		constexpr Flags<T> operator^(Flags<T> lhs, T rhs)
		{
			return lhs ^ Flags(rhs);
		}

		template<Concepts::Enum T>
		constexpr Flags<T> operator^(T lhs, Flags<T> rhs)
		{
			return Flags(lhs) ^ rhs;
		}

		template<Concepts::Enum T>
		constexpr Flags<T> operator~(Flags<T> rhs)
		{
			return static_cast<T>(~rhs.Underlying());
		}

		template<Concepts::Enum T>
		constexpr Flags<T>& operator|=(Flags<T>& lhs, Flags<T> rhs)
		{
			return lhs = lhs | rhs;
		}

		template<Concepts::Enum T>
		constexpr Flags<T>& operator|=(Flags<T>& lhs, T rhs)
		{
			return lhs = lhs | Flags(rhs);
		}

		template<Concepts::Enum T>
		constexpr T& operator|=(T& lhs, Flags<T> rhs)
		{
			return lhs = (Flags(lhs) | rhs).Enum();
		}

		template<Concepts::Enum T>
		constexpr Flags<T>& operator&=(Flags<T>& lhs, Flags<T> rhs)
		{
			return lhs = lhs & rhs;
		}

		template<Concepts::Enum T>
		constexpr Flags<T>& operator&=(Flags<T>& lhs, T rhs)
		{
			return lhs = lhs & Flags(rhs);
		}

		template<Concepts::Enum T>
		constexpr T& operator&=(T& lhs, Flags<T> rhs)
		{
			return lhs = (Flags(lhs) & rhs).Enum();
		}

	}

}

namespace Shark::Enum {

	template<Concepts::Enum T>
	constexpr bool Flags<T>::Any() const
	{
		return Underlying() != 0;
	}

	template<Concepts::Enum T>
	constexpr bool Flags<T>::Has(T flag) const
	{
		return static_cast<T>(Underlying() & Underlying(flag)) == flag;
	}

	template<Concepts::Enum T>
	constexpr Flags<T>& Flags<T>::Set(T flag)
	{
		m_Flags = static_cast<T>(Underlying() | Underlying(flag));
		return *this;
	}

	template<Concepts::Enum T>
	constexpr Flags<T>& Flags<T>::Set(T flag, bool set)
	{
		if (set)
			m_Flags = static_cast<T>(Underlying() | Underlying(flag));
		else
			m_Flags = static_cast<T>(Underlying() & ~Underlying(flag));
		return *this;
	}

}
