#pragma once

#include <glm/glm.hpp>

namespace Shark {
	struct TransformComponent;
}

namespace Shark::Math {

	bool DecomposeTransform(const glm::mat4& ModelMatrix, glm::vec3& out_Translation, glm::vec3& out_Euler, glm::vec3& out_Scale);
	bool DecomposeTransform(const glm::mat4& ModelMatrix, TransformComponent& out_Transform);
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

}
