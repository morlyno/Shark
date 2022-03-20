#pragma once

#include <glm/glm.hpp>
#include <filesystem>

namespace Shark::Utility {

	inline std::filesystem::path CreatePathFormIterator(const std::filesystem::path::const_iterator& begin, const std::filesystem::path::const_iterator& end)
	{
		std::filesystem::path path;
		for (auto it = begin; it != end; ++it)
			path /= *it;
		return path;
	}

	template<typename T>
	inline const T* ValuePtr(const T& val)
	{
		static_assert(std::is_arithmetic_v<T>);
		return &val;
	}

	template<typename T>
	inline T* ValuePtr(T& val)
	{
		static_assert(std::is_arithmetic_v<T>);
		return &val;
	}

	template<glm::length_t Length, typename T, glm::qualifier Q>
	inline const T* ValuePtr(const glm::vec<Length, T, Q>& val)
	{
		return glm::value_ptr(val);
	}

	template<glm::length_t Length, typename T, glm::qualifier Q>
	inline T* ValuePtr(glm::vec<Length, T, Q>& val)
	{
		return glm::value_ptr(val);
	}

	template<typename Container>
	bool Contains(const Container& container, const typename Container::value_type& value)
	{
		const auto iter = std::find(container.cbegin(), container.cend(), value);
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

	template<typename Key, typename T>
	bool Contains(const std::unordered_set<Key, T>& map, const Key& key)
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

	constexpr glm::vec4 UI32ToF4(uint32_t color)
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

	template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
	auto EnumToInt(const T& e)
	{
		return (std::underlying_type_t<T>)e;
	}
	
	template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
	auto& EnumToInt(T& e)
	{
		return (std::underlying_type_t<T>&)e;
	}

}