#include "skpch.h"
#include "Utility.h"

#include <filesystem>

namespace Shark::Utility {

	ImVec4 ToImVec4(const DirectX::XMFLOAT4& color)
	{
		return ImVec4{ color.x, color.y, color.z, color.w };
	}

	std::vector<std::string> StringSplit(const std::string& str, const std::string& splitter)
	{
		std::vector<std::string> strings;
		size_t offset = 0;
		size_t end = 0;
		while (end != std::string::npos)
		{
			end = str.find_first_of(splitter, offset);
			strings.emplace_back(str.substr(offset, end - offset));
			offset = end + 1;
		}
		return std::move(strings);
	}

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

}