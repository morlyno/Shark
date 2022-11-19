#pragma once

namespace Shark::String {

	std::string ToLowerCopy(const std::string& str);
	void ToLower(std::string& str);

	std::string ToNarrowCopy(const std::wstring& str);
	std::string ToNarrowCopy(std::wstring_view str);
	std::string ToNarrowCopy(const wchar_t* str);
	void ToNarrow(const std::wstring& str, std::string& out_Result);
	void ToNarrow(std::wstring_view str, std::string& out_Result);

	std::wstring ToWideCopy(const std::string& str);
	void ToWide(const std::string& str, std::wstring& out_Result);

	void SplitString(const std::string& str, std::string_view splitter, std::vector<std::string>& out_Array);
	void SplitString(const std::wstring& str, std::wstring_view splitter, std::vector<std::wstring>& out_Array);

	void Replace(std::string& str, std::string_view from, std::string_view to);

	void RemovePrefix(std::string& str, uint32_t count);
	void RemoveSuffix(std::string& str, uint32_t count);

	void Strip(std::string& str, char c = ' ');
	void StripBack(std::string& str, char c = ' ');
	void StripFront(std::string& str, char c = ' ');

	std::filesystem::path FormatWindowsCopy(const std::filesystem::path& path);
	void FormatWindows(std::filesystem::path& path);

	std::filesystem::path FormatDefaultCopy(const std::filesystem::path& path);
	void FormatDefault(std::filesystem::path& path);

	bool IsDefaultFormat(const std::filesystem::path& path);

}
