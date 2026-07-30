#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Shark {

	struct Transform
	{
		glm::vec3 Translation;
		glm::quat Rotation;
		float Scale;

		static Transform Identity()
		{
			return {
				glm::vec3(0.0f),
				glm::identity<glm::quat>(),
				1.0f
			};
		}
	};

}
