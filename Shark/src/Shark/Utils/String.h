#pragma once

namespace Shark {

	enum class Case
	{
		Sensitive,
		Ingnore
	};

}

namespace Shark::String {

	bool Contains(std::string_view text, std::string_view pattern, bool caseSensitive = true);
	bool Compare(std::string_view lhs, std::string_view rhs, Case comapreCase);

	std::string ToLowerCopy(const std::string& str);
	void ToLower(std::string& str);

	std::string ToNarrow(const std::wstring& str);
	std::string ToNarrow(std::wstring_view str);
	std::string ToNarrow(const wchar_t* str);
	std::wstring ToWide(const std::string& str);

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
