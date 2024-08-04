#pragma once

#include <fmt/format.h>
#include <glm/glm.hpp>

template<glm::length_t L, typename T, glm::qualifier Q, typename Char>
struct fmt::formatter<glm::vec<L, T, Q>, Char> : fmt::formatter<T, Char>
{
	template<typename FormatContext>
	auto format(const glm::vec<L, T, Q>& val, FormatContext& ctx) const -> decltype(ctx.out())
	{
		auto&& out = ctx.out();
		fmt::detail::write(out, "[");
		
		for (auto i = 0; i < (L - 1); i++)
		{
			fmt::formatter<T, Char>::format(val[i], ctx);
			fmt::detail::write(out, ", ");
		}
		fmt::formatter<T, Char>::format(val[L - 1], ctx);

		fmt::detail::write(out, "]");

		return out;
	}

};

#if 0
template<>
struct fmt::formatter<std::filesystem::path, wchar_t> : fmt::formatter<std::wstring, wchar_t>
{
	template<typename FormatContext>
	auto format(const std::filesystem::path& path, FormatContext& ctx) const -> decltype(ctx.out())
	{
		return fmt::formatter<std::wstring, wchar_t>::format(path.native(), ctx);
	}
};

template<>
struct fmt::formatter<std::filesystem::path, char> : fmt::formatter<std::string, char>
{
	template<typename FormatContext>
	auto format(const std::filesystem::path& path, FormatContext& ctx) const -> decltype(ctx.out())
	{
		return fmt::formatter<std::string, char>::format(path.string(), ctx);
	}
};
#endif

template<typename TEnum>
	requires std::is_enum_v<TEnum>
struct fmt::formatter<TEnum, char>
{
	constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
	{
		return ctx.end();
	}

	template<typename FormatContext>
	auto format(TEnum value, FormatContext& ctx) const -> decltype(ctx.out())
	{
		return fmt::format_to(ctx.out(), "{}", magic_enum::enum_name(value));
	}
};
