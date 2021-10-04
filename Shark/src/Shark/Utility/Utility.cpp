#include "skpch.h"
#include "Utility.h"

#include <filesystem>

namespace Shark::Utility {

	std::string ToLower(const std::string& src)
	{
		std::string str;
		str.resize(src.size());
		std::transform(src.cbegin(), src.cend(), str.begin(), [](auto c)
		{
			return std::tolower(c);
		});
		return str;
	}

	std::filesystem::path CreatePathFormIterator(const std::filesystem::path::const_iterator& begin, const std::filesystem::path::const_iterator& end)
	{
		std::filesystem::path path;
		for (auto it = begin; it != end; ++it)
			path /= *it;
		return std::move(path);
	}

	template<>
	float* GetValuePtr(const DirectX::XMFLOAT4& vec)
	{
		return (float*)&vec;
	}

	template<>
	float* GetValuePtr(const DirectX::XMFLOAT3& vec)
	{
		return (float*)&vec;
	}

	template<>
	float* GetValuePtr(const DirectX::XMFLOAT2& vec)
	{
		return (float*)&vec;
	}

	std::string_view GetPathName(std::string_view path)
	{
#if SK_DEBUG
		std::string_view name = path;
		auto namebeg = path.find_last_of("/\\");
		if (namebeg != std::string_view::npos)
			name = path.substr(namebeg + 1);
		SK_CORE_ASSERT(name.empty() ? true : name.back() != '\0', "String View dosen't end with \0");
		return name;
#else
		auto namebeg = path.find_last_of("/\\");
		if (namebeg != std::string_view::npos)
			return path.substr(namebeg + 1);
		return path;
#endif
	}

	std::string_view GetFileExtention(std::string_view path)
	{
#if SK_DEBUG
		std::string_view extention;
		auto offset = path.find_last_of("/\\");
		auto extbeg = path.find_first_of(".", offset);
		if (extbeg != std::string_view::npos)
			extention = path.substr(extbeg);
		SK_CORE_ASSERT(extention.empty() ? true : extention.back() != '\0', "String View dosen't end with \0");
		return extention;
#else
		auto offset = path.find_last_of("/\\");
		auto extbeg = path.find_first_of(".", offset);
		if (extbeg != std::string_view::npos)
			return path.substr(extbeg);
		return std::string_view{};
#endif
	}

	std::string ToNarrow(const std::wstring& str)
	{
		return std::string(str.begin(), str.end());
	}

}