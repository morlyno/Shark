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
		else if (comapreCase == Case::Ignore)
			comparer = [](const auto& lhs, const auto& rhs) { return std::tolower(lhs) == std::tolower(rhs); };

		return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), comparer);
	}

	bool StartsWith(std::string_view lhs, std::string_view rhs, Case compareCase)
	{
		if (lhs.length() < rhs.length())
			return false;

		lhs.remove_suffix(lhs.length() - rhs.length());
		return Compare(lhs, rhs, compareCase);
	}

	bool EndsWith(std::string_view lhs, std::string_view rhs, Case compareCase)
	{
		if (lhs.length() < rhs.length())
			return false;

		lhs.remove_prefix(lhs.length() - rhs.length());
		return Compare(lhs, rhs, compareCase);
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

	std::string ToNarrow(const std::wstring& wide)
	{
		std::string narrow;
		narrow.resize(wide.size() * 2);
		size_t actual;
		wcstombs_s(&actual, narrow.data(), narrow.size(), wide.c_str(), _TRUNCATE);
		narrow.resize(actual - 1);
		return narrow;
	}

	std::string ToNarrow(std::wstring_view wide)
	{
		std::string narrow;
		narrow.resize(wide.size() * 2);
		size_t actual;
		wcstombs_s(&actual, narrow.data(), narrow.size(), wide.data(), _TRUNCATE);
		narrow.resize(actual - 1);
		return narrow;
	}

	std::string ToNarrow(const wchar_t* wide)
	{
		std::string narrow;
		size_t wideLength = wcslen(wide);
		narrow.resize(wideLength * 2);
		size_t actual;
		wcstombs_s(&actual, narrow.data(), narrow.size(), wide, _TRUNCATE);
		narrow.resize(actual - 1);
		return narrow;
	}

	std::wstring ToWide(const std::string& narrow)
	{
		std::wstring wide;
		wide.resize(narrow.size() + 1);
		size_t actual;
		mbstowcs_s(&actual, wide.data(), wide.size(), narrow.c_str(), _TRUNCATE);
		if (actual > 0)
		{
			wide.resize(actual - 1);
			return wide;
		}
		return {};
	}

	void SplitString(std::string_view str, std::string_view splitter, std::vector<std::string>& out_Array)
	{
		Strip(str, splitter);
		if (str.empty())
			return;

		size_t start = str.find_first_not_of(splitter);
		size_t end = 0;
		while (end != std::string_view::npos)
		{
			end = str.find(splitter, start);
			out_Array.emplace_back(str.substr(start, end - start));

			start = str.find_first_not_of(splitter, end + 1);
		}
	}

	void SplitString(std::wstring_view str, std::wstring_view splitter, std::vector<std::wstring>& out_Array)
	{
		Strip(str, splitter);
		if (str.empty())
			return;
		
		size_t start = str.find_first_not_of(splitter);
		size_t end = 0;
		while (end != std::wstring_view::npos)
		{
			end = str.find(splitter, start);
			out_Array.emplace_back(str.substr(start, end - start));

			start = str.find_first_not_of(splitter, end + 1);
		}
	}

	std::vector<std::string> SplitString(std::string_view str, std::string_view splitter)
	{
		std::vector<std::string> result;
		SplitString(str, splitter, result);
		return result;
	}

	std::vector<std::wstring> SplitString(std::wstring_view str, std::wstring_view splitter)
	{
		std::vector<std::wstring> result;
		SplitString(str, splitter, result);
		return result;
	}

	void SplitToRanges(std::string_view str, std::string_view splitter, std::vector<std::string_view>& outArray)
	{
		Strip(str, splitter);
		if (str.empty())
			return;

		size_t start = 0;
		size_t end = 0;
		while (end != std::string_view::npos)
		{
			end = str.find(splitter, start);
			outArray.emplace_back(str.substr(start, end - start));
			
			start = str.find_first_not_of(splitter, end + 1);
		}
	}

	std::vector<std::string_view> SplitToRanges(std::string_view str, std::string_view splitter)
	{
		std::vector<std::string_view> result;
		SplitToRanges(str, splitter, result);
		return result;
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

	void Remove(std::string& str, std::string_view pattern)
	{
		if (pattern.empty())
			return;

		size_t pos = 0;
		while ((pos = str.find(pattern, pos)) != std::string::npos)
		{
			str.erase(pos, pattern.length());
		}
	}

	void RemoveFirst(std::string& str, std::string_view pattern)
	{
		if (pattern.empty())
			return;

		size_t pos = 0;
		if ((pos = str.find(pattern, pos)) != std::string::npos)
		{
			str.erase(pos, pattern.length());
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

	void Strip(std::string& str, std::string_view chars)
	{
		StripFront(str, chars);
		StripBack(str, chars);
	}

	void StripBack(std::string& str, std::string_view chars)
	{
		size_t offset = str.find_last_not_of(chars);
		if (offset != std::string::npos)
			str.erase(offset + 1);
		else
			str.clear();
	}

	void StripFront(std::string& str, std::string_view chars)
	{
		size_t end = str.find_first_not_of(chars);
		if (end != std::string::npos)
			str.erase(0, end);
		else
			str.clear();
	}

	void Strip(std::string_view& str, std::string_view chars)
	{
		StripFront(str, chars);
		StripBack(str, chars);
	}

	void StripBack(std::string_view& str, std::string_view chars)
	{
		size_t offset = str.find_last_not_of(chars);
		if (offset != std::string_view::npos)
			str = str.substr(0, offset + 1);
		else
			str = {};
	}

	void StripFront(std::string_view& str, std::string_view chars)
	{
		size_t offset = str.find_first_not_of(chars);
		if (offset != std::string_view::npos)
			str = str.substr(offset);
		else
			str = {};
	}

	void Strip(std::wstring_view& str, std::wstring_view chars)
	{
		StripFront(str, chars);
		StripBack(str, chars);
	}

	void StripBack(std::wstring_view& str, std::wstring_view chars)
	{
		size_t offset = str.find_last_not_of(chars);
		if (offset != std::wstring_view::npos)
			str = str.substr(0, offset + 1);
		else
			str = {};
	}

	void StripFront(std::wstring_view& str, std::wstring_view chars)
	{
		size_t offset = str.find_first_not_of(chars);
		if (offset != std::wstring_view::npos)
			str = str.substr(offset);
		else
			str = {};
	}

	std::string BytesToString(uint64_t bytes)
	{
		static constexpr uint64_t TB = 1024ull * 1024 * 1024 * 1024;
		static constexpr uint64_t GB = 1024 * 1024 * 1024;
		static constexpr uint64_t MB = 1024 * 1024;
		static constexpr uint64_t KB = 1024;

		const auto sizes = {
			std::pair{ TB, "TB"sv },
			std::pair{ GB, "GB"sv },
			std::pair{ MB, "MB"sv },
			std::pair{ KB, "KB"sv }
		};

		for (const auto& [size, suffix] : sizes)
		{
			if (bytes >= size)
			{
				return fmt::format("{0:.2f} {1}", (float)bytes / size, suffix);
			}
		}
		return fmt::format("{0} bytes", bytes);

#if 0
		if (bytes > (1024 * 1024 * 1024))
			return fmt::format("{0:.2f} GB", (float)bytes / (1024 * 1024 * 1024));

		if (bytes > (1024 * 1024))
			return fmt::format("{0:.2f} MB", (float)bytes / (1024 * 1024));

		if (bytes > (1024))
			return fmt::format("{0:.2f} KB", (float)bytes / (1024));

		return fmt::format("{0} bytes", bytes);
#endif
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
