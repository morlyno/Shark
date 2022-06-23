#pragma once

#include <Shark/Core/Base.h>
#include <Shark/Core/UUID.h>

#include <fmt/format.h>
#include <glm/glm.hpp>


template<glm::length_t L, typename T, glm::qualifier Q, typename Char>
struct fmt::formatter<glm::vec<L, T, Q>, Char> : fmt::formatter<T, Char>
{
	template<typename FormatContext>
	auto format(const glm::vec<L, T, Q>& val, FormatContext& ctx) -> decltype(ctx.out())
	{
		auto&& out = ctx.out();
		format_to(out, "[");
		
		for (auto i = 0; i < (L - 1); i++)
		{
			fmt::formatter<T, Char>::format(val[i], ctx);
			format_to(out, ", ");
		}
		fmt::formatter<T, Char>::format(val[L - 1], ctx);

		format_to(out, "]");

		return out;
	}

};

template<>
struct fmt::formatter<std::filesystem::path, wchar_t> : fmt::formatter<std::wstring, wchar_t>
{
	template<typename FormatContext>
	auto format(const std::filesystem::path& path, FormatContext& ctx) -> decltype(ctx.out())
	{
		return fmt::formatter<std::wstring, wchar_t>::format(path.native(), ctx);
	}
};

template<>
struct fmt::formatter<std::filesystem::path, char> : fmt::formatter<std::string, char>
{
	template<typename FormatContext>
	auto format(const std::filesystem::path& path, FormatContext& ctx) -> decltype(ctx.out())
	{
		return fmt::formatter<std::string, char>::format(path.string(), ctx);
	}
};

template<typename Char>
struct fmt::formatter<Shark::UUID, Char> : fmt::formatter<uint64_t, Char>
{
	template<typename FormatContext>
	auto format(const Shark::UUID& uuid, FormatContext& ctx) -> decltype(ctx.out())
	{
		return fmt::formatter<uint64_t, Char>::format((uint64_t)uuid, ctx);
	}
};
