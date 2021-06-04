#include "skpch.h"
#include "Utility.h"

#include <filesystem>

#include "Shark/Render/Material.h"

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

	std::filesystem::path MakeAbsolutePathRelative(const std::filesystem::path& path)
	{
		SK_CORE_ASSERT(path.is_absolute(), "Path must be absolute");
		auto& str = path.string();
		auto&& currPath = std::filesystem::current_path().string();
		return str.substr(currPath.length() + 1);
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

	uint32_t GetSizeFromDataType(DataType type)
	{
		switch (type)
		{
			case DataType::Void:   return 0;
			case DataType::Bool:   return 4;
			case DataType::Int:    return 4;
			case DataType::Float:  return 4;
		}
		SK_CORE_ASSERT(false);
		return 0;
	}


}