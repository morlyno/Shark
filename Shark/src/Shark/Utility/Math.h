#pragma once

#include <glm/glm.hpp>

namespace Shark::Math {

	bool Decompose(const glm::mat4& ModelMatrix, glm::vec3& Translation, glm::vec3& Rotation, glm::vec3& Scale);

	bool DecomposeTranslation(const glm::mat4& ModelMatrix, glm::vec3& Translation);
	bool DecomposeRotation(const glm::mat4& ModelMatrix, glm::vec3& Rotation);
	bool DecomposeScale(const glm::mat4& ModelMatrix, glm::vec3& Scale);

}
