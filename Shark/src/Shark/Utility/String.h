#pragma once

namespace Shark::String {

	std::string ToLowerCopy(const std::string& str);
	void ToLower(std::string& str);

	std::string ToNarrowCopy(const std::wstring& str);
	void ToNarrow(const std::wstring& str, std::string& out_Result);

	std::wstring ToWideCopy(const std::string& str);
	void ToWide(const std::string& str, std::wstring& out_Result);

	void SplitString(const std::string& str, std::string_view splitter, std::vector<std::string>& out_Array);
	void SplitString(const std::wstring& str, std::wstring_view splitter, std::vector<std::wstring>& out_Array);

}
