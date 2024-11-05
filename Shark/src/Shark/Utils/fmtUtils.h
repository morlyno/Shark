#pragma once

#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/std.h>
#include <fmt/ranges.h>

#include <glm/glm.hpp>
#include <magic_enum_format.hpp>

template<typename T, glm::qualifier Q>
struct fmt::formatter<glm::vec<2, T, Q>> : fmt::formatter<T>
{
	using UnderlyingFormatter = fmt::formatter<T>;

	auto format(const glm::vec<2, T, Q>& val, fmt::format_context& ctx) const -> fmt::format_context::iterator
	{
		fmt::detail::write(ctx.out(), '[');
		UnderlyingFormatter::format(val.x, ctx);
		fmt::detail::write(ctx.out(), ", ");
		UnderlyingFormatter::format(val.y, ctx);
		fmt::detail::write(ctx.out(), ']');
		return ctx.out();
	}
};

template<typename T, glm::qualifier Q>
struct fmt::formatter<glm::vec<3, T, Q>> : fmt::formatter<T>
{
	using UnderlyingFormatter = fmt::formatter<T>;

	auto format(const glm::vec<3, T, Q>& val, fmt::format_context& ctx) const -> fmt::format_context::iterator
	{
		fmt::detail::write(ctx.out(), '[');
		UnderlyingFormatter::format(val.x, ctx);
		fmt::detail::write(ctx.out(), ", ");
		UnderlyingFormatter::format(val.y, ctx);
		fmt::detail::write(ctx.out(), ", ");
		UnderlyingFormatter::format(val.y, ctx);
		fmt::detail::write(ctx.out(), ']');
		return ctx.out();
	}
};

template<typename T, glm::qualifier Q>
struct fmt::formatter<glm::vec<4, T, Q>> : fmt::formatter<T>
{
	using UnderlyingFormatter = fmt::formatter<T>;

	auto format(const glm::vec<4, T, Q>& val, fmt::format_context& ctx) const -> fmt::format_context::iterator
	{
		fmt::detail::write(ctx.out(), '[');
		UnderlyingFormatter::format(val.x, ctx);
		fmt::detail::write(ctx.out(), ", ");
		UnderlyingFormatter::format(val.y, ctx);
		fmt::detail::write(ctx.out(), ", ");
		UnderlyingFormatter::format(val.y, ctx);
		fmt::detail::write(ctx.out(), ", ");
		UnderlyingFormatter::format(val.y, ctx);
		fmt::detail::write(ctx.out(), ']');
		return ctx.out();
	}
};
