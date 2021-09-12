#pragma once

#include <imgui.h>
#include <DirectXMath.h>
#include <filesystem>

namespace Shark::Utility {

	ImVec4 ToImVec4(const DirectX::XMFLOAT4& color);

	std::string ToLower(const std::string& src);

	std::filesystem::path CreatePathFormIterator(const std::filesystem::path::const_iterator& begin, const std::filesystem::path::const_iterator& end);

	template<typename T>
	float* GetValuePtr(const T& vec);

	std::string_view GetPathName(std::string_view path);

	std::string_view GetFileExtention(std::string_view path);

	template<typename Container>
	bool Contains(const Container& container, const typename Container::value_type& value)
	{
		const auto iter = std::find_if(container.cbegin(), container.cend(), [&value](const auto& other)
		{
			return value == other;
		});
		if (iter == container.cend())
			return false;
		return true;
	}

	template<typename Container>
	void Erase(Container& container, size_t index)
	{
		container.erase(container.cbegin() + index);
	}

	template<typename T, uint32_t _Count>
	constexpr uint32_t ArraySize(const T(&)[_Count]) { return _Count; }

}