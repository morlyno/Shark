#pragma once

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Shark {

	struct TransformComponent
	{
		glm::mat4 CalcTransform() const
		{
			return glm::translate(glm::mat4(1), Translation) *
				glm::toMat4(glm::quat(Rotation)) *
				glm::scale(glm::mat4(1), Scale);
		}

		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };
	};

}