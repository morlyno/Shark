#pragma once

#include <DirectXMath.h>
#include <filesystem>

namespace Shark::String {

	std::string ToLowerCopy(const std::string& str);
	void ToLower(std::string& str);

}

namespace Shark::Utility {

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

	template<typename Key, typename T>
	bool Contains(const std::unordered_map<Key, T>& map, const Key& key)
	{
		const auto iter = map.find(key);
		if (iter == map.cend())
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

	std::string ToNarrow(const std::wstring& str);
	std::wstring ToWide(const std::string& str);

	constexpr DirectX::XMFLOAT4 UI32ToF4(uint32_t color)
	{
		return {
			(float)((color >>  0) & 0xFF) / 255.0f,
			(float)((color >>  8) & 0xFF) / 255.0f,
			(float)((color >> 16) & 0xFF) / 255.0f,
			(float)((color >> 24) & 0xFF) / 255.0f,
		};
	}

	template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
	T Wrap(const T& val, const T& max)
	{
		return val % max;
	}

	template<typename String>
	std::string ToStdString(const String& str)
	{
		if constexpr (std::is_same_v<String, std::wstring>)
			return ToNarrow(str);
		else if constexpr (std::is_same_v<String, std::filesystem::path>)
			return str.string();
		else
			return str;
	}

	void SplitString(const std::string& str, std::string_view splitter, std::vector<std::string>& out_Array);
	void SplitString(const std::wstring& str, std::wstring_view splitter, std::vector<std::wstring>& out_Array);

}