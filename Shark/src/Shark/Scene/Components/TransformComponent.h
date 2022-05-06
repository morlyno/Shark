#pragma once

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Shark {

	struct TransformComponent
	{
		glm::mat4 GetTranform() const
		{
			return glm::translate(glm::mat4(1), Position) *
				glm::toMat4(glm::quat(Rotation)) *
				glm::scale(glm::mat4(1), Scaling);
		}

		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scaling = { 1.0f, 1.0f, 1.0f };
	};

}