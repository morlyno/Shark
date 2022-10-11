#include "skpch.h"
#include "String.h"

namespace Shark::String {

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
