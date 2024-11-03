#pragma once

#include <glm/glm.hpp>

namespace Shark::Math {

	constexpr float PI = 3.14159265358979323846264338327950288f;

	constexpr float Deg2Rad = 0.01745329251994329576923690768489f;
	constexpr float Rad2Deg = 57.295779513082320876798154814105f;

	bool DecomposeTransform(const glm::mat4& ModelMatrix, glm::vec3& out_Translation, glm::vec3& out_Euler, glm::vec3& out_Scale);
	bool DecomposeTranslation(const glm::mat4& ModelMatrix, glm::vec3& out_Translation);

	template <class T, std::enable_if_t<std::_Is_standard_unsigned_integer<T>, int> = 0>
	constexpr bool SingleBitSet(const T& val)
	{
		return val != 0 && (val & (val - 1)) == 0;
	}

	template <class T, std::enable_if_t<std::is_enum_v<T> && std::_Is_standard_unsigned_integer<std::underlying_type_t<T>>, int> = 0>
	constexpr bool SingleBitSet(const T& val)
	{
		return val != 0 && (val & (val - 1)) == 0;
	}

	bool IsWhole(float val);

}
