#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Shark {

	struct Transform
	{
		glm::vec3 Translation;
		glm::quat Rotation;
		float Scale;
	};

}
