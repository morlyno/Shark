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

	void ToNarrow(const std::wstring& str, std::string& out_Result)
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

}
