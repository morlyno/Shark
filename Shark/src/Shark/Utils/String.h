#pragma once

#include <filesystem>
#include <regex>

namespace Shark::String {

	namespace RegexLiterals {

		inline std::regex operator""_r(const char* str, size_t len) { return std::regex(str, len); }

	}

	enum class Case
	{
		Sensitive,
		Ignore
	};

	bool Contains(std::string_view text, std::string_view pattern, bool caseSensitive = true);
	bool Compare(std::string_view lhs, std::string_view rhs, Case compareCase);

	bool StartsWith(std::string_view lhs, std::string_view rhs, Case compareCase = Case::Sensitive);
	bool EndsWith(std::string_view lhs, std::string_view rhs, Case compareCase = Case::Sensitive);

	std::string ToLowerCopy(const std::string& str);
	void ToLower(std::string& str);

	std::string ToNarrow(const std::wstring& str);
	std::string ToNarrow(std::wstring_view str);
	std::string ToNarrow(const wchar_t* str);
	std::wstring ToWide(const std::string& str);
	std::wstring ToWide(std::string_view str);

	void SplitString(std::string_view str, std::string_view splitter, std::vector<std::string>& out_Array);
	void SplitString(std::wstring_view str, std::wstring_view splitter, std::vector<std::wstring>& out_Array);

	std::vector<std::string> SplitString(std::string_view str, std::string_view splitter);
	std::vector<std::wstring> SplitString(std::wstring_view str, std::wstring_view splitter);

	void SplitToRanges(std::string_view str, std::string_view splitter, std::vector<std::string_view>& outArray);
	std::vector<std::string_view> SplitToRanges(std::string_view str, std::string_view splitter);

	void Replace(std::string& str, std::string_view from, std::string_view to);
	void Remove(std::string& str, std::string_view pattern);
	void RemoveFirst(std::string& str, std::string_view pattern);

	void RemovePrefix(std::string& str, uint32_t count);
	void RemoveSuffix(std::string& str, uint32_t count);

	void Strip(std::string& str, std::string_view chars = " ");
	void StripBack(std::string& str, std::string_view chars = " ");
	void StripFront(std::string& str, std::string_view chars = " ");

	void Strip(std::string_view& str, std::string_view chars);
	void StripBack(std::string_view& str, std::string_view chars);
	void StripFront(std::string_view& str, std::string_view chars);
	
	void Strip(std::wstring_view& str, std::wstring_view chars);
	void StripBack(std::wstring_view& str, std::wstring_view chars);
	void StripFront(std::wstring_view& str, std::wstring_view chars);

	std::string BytesToString(uint64_t bytes);

	std::filesystem::path FormatWindowsCopy(const std::filesystem::path& path);
	void FormatWindows(std::filesystem::path& path);

	std::filesystem::path FormatDefaultCopy(const std::filesystem::path& path);
	void FormatDefault(std::filesystem::path& path);

	bool IsDefaultFormat(const std::filesystem::path& path);

	constexpr void Trim(std::string_view& str, std::string_view chars = " ")
	{
		size_t start = str.find_first_not_of(chars);
		size_t end = str.find_last_not_of(chars);

		if (start == std::string_view::npos)
			start = 0;

		if (end == std::string_view::npos)
			end = str.size() - 1;

		str = str.substr(start, end - start + 1);
	}

	template<size_t N>
	constexpr std::array<std::string_view, N> SplitString(std::string_view source, std::string_view delimiter)
	{
		std::array<std::string_view, N> tokens;
		if (source.empty())
			return tokens;

		size_t start = source.find_first_not_of(delimiter);
		size_t end = 0;
		size_t i = 0;
		while (end != std::wstring_view::npos)
		{
			end = source.find(delimiter, start);
			tokens[i] = source.substr(start, end - start);

			start = source.find_first_not_of(delimiter, end + 1);
			i++;
		}

		return tokens;
	}

}
