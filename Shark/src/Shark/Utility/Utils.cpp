#include <skpch.h>
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

}