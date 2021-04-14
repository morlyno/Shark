#include "skpch.h"
#include "Utils.h"

#include "Shark/Core/Application.h"
#include <filesystem>

namespace Shark::Utils {

	std::string MakePathRelative(const std::string& filepath)
	{
		std::filesystem::path path = filepath;
		std::string rootdir = Application::GetRootDirectory();
		auto relativePath = path.lexically_relative(rootdir).string();
		return relativePath;
	}

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

}