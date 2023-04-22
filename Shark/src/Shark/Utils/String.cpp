#include "skpch.h"
#include "String.h"
#include <functional>

namespace Shark::String {

	bool Contains(std::string_view text, std::string_view pattern, bool caseSensitive)
	{
		bool(*comparer)(const char& lhs, const char& rhs);

		if (caseSensitive)
			comparer = [](const auto& lhs, const auto& rhs) { return lhs == rhs; };
		else
			comparer = [](const auto& lhs, const auto& rhs) { return std::tolower(lhs) == std::tolower(rhs); };

		std::default_searcher searcher(pattern.begin(), pattern.end(), comparer);
		const auto i = std::search(text.begin(), text.end(), searcher);
		return i != text.end();
	}

	bool Compare(std::string_view lhs, std::string_view rhs, Case comapreCase)
	{
		bool(*comparer)(const char& lhs, const char& rhs);

		if (comapreCase == Case::Sensitive)
			comparer = [](const auto& lhs, const auto& rhs) { return lhs == rhs; };
		else if (comapreCase == Case::Ingnore)
			comparer = [](const auto& lhs, const auto& rhs) { return std::tolower(lhs) == std::tolower(rhs); };

		return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), comparer);
	}

	std::string ToLowerCopy(const std::string& str)
	{
		std::string result = str;
		ToLower(result);
		return result;
	}

	void ToLower(std::string& str)
	{
		std::transform(str.begin(), str.end(), str.begin(), [](auto c)
		{
			return std::tolower(c);
		});
	}

	std::string ToNarrowCopy(const std::wstring& str)
	{
		std::string result;
		ToNarrow(str, result);
		return std::move(result);
	}

	std::string ToNarrowCopy(std::wstring_view str)
	{
		std::string result;
		ToNarrow(str, result);
		return std::move(result);
	}

	std::string ToNarrowCopy(const wchar_t* str)
	{
		return ToNarrowCopy(std::wstring_view(str));
	}

	void ToNarrow(const std::wstring& str, std::string& out_Result)
	{
		out_Result.resize(str.size());
		wcstombs_s(nullptr, out_Result.data(), out_Result.size() + 1, str.data(), (str.size() + 1) * sizeof(std::wstring::value_type));
	}

	void ToNarrow(std::wstring_view str, std::string& out_Result)
	{
		out_Result.resize(str.size());
		wcstombs_s(nullptr, out_Result.data(), out_Result.size() + 1, str.data(), (str.size() + 1) * sizeof(std::wstring::value_type));
	}

	std::wstring ToWideCopy(const std::string& str)
	{
		std::wstring result;
		ToWide(str, result);
		return result;
	}

	void ToWide(const std::string& str, std::wstring& out_Result)
	{
		out_Result.resize(str.size());
		mbstowcs_s(nullptr, out_Result.data(), out_Result.size() + 1, str.data(), (str.size() + 1) * sizeof(std::wstring::value_type));
	}

	void SplitString(const std::string& str, std::string_view splitter, std::vector<std::string>& out_Array)
	{
		size_t start = str.find_first_not_of(' ');
		size_t end = 0;
		while (end != std::string::npos)
		{
			end = str.find(splitter, start);
			out_Array.emplace_back(str.substr(start, end - start));

			start = str.find_first_not_of(' ', end + 1);
		}
	}

	void SplitString(const std::wstring& str, std::wstring_view splitter, std::vector<std::wstring>& out_Array)
	{
		size_t start = str.find_first_not_of(' ');
		size_t end = 0;
		while (end != std::wstring::npos)
		{
			end = str.find(splitter, start);
			out_Array.emplace_back(str.substr(start, end - start));

			start = str.find_first_not_of(' ', end + 1);
		}
	}

	void Replace(std::string& str, std::string_view from, std::string_view to)
	{
		if (from.empty())
			return;

		size_t pos = 0;
		while ((pos = str.find(from, pos)) != std::string::npos)
		{
			str.replace(pos, from.length(), to);
			pos += to.length();
		}
	}

	void RemovePrefix(std::string& str, uint32_t count)
	{
		str.erase(0, count);
	}

	void RemoveSuffix(std::string& str, uint32_t count)
	{
		str.erase(str.size() - count, std::string::npos);
	}

	void Strip(std::string& str, char c)
	{
		StripFront(str, c);
		StripBack(str, c);
	}

	void StripBack(std::string& str, char c)
	{
		size_t offset = str.find_last_not_of(c);
		if (offset != std::string::npos)
			str.erase(offset + 1);
	}

	void StripFront(std::string& str, char c)
	{
		size_t end = str.find_first_not_of(c);
		if (end != std::string::npos)
			str.erase(0, end);
	}

	std::filesystem::path FormatWindowsCopy(const std::filesystem::path& path)
	{
		std::wstring str = path.native();
		std::replace(str.begin(), str.end(), L'/', L'\\');
		return str;
	}

	void FormatWindows(std::filesystem::path& path)
	{
		std::wstring str = path.wstring();
		std::replace(str.begin(), str.end(), L'/', L'\\');
		path = str;
	}

	std::filesystem::path FormatDefaultCopy(const std::filesystem::path& path)
	{
		std::wstring str = path.wstring();
		std::replace(str.begin(), str.end(), L'\\', L'/');
		return str;
	}

	void FormatDefault(std::filesystem::path& path)
	{
		std::wstring str = path.wstring();
		std::replace(str.begin(), str.end(), L'\\', L'/');
		path = str;
	}

	bool IsDefaultFormat(const std::filesystem::path& path)
	{
		const std::wstring& str = path;
		return str.find(L'\\') == std::wstring::npos;
	}

}
