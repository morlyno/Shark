#pragma once

#include <glm/glm.hpp>

namespace Shark {

	class Physics2DActor;

	class Physics2DUtils
	{
	public:
		static glm::mat4 GetMatrix(Ref<Physics2DActor> actor);
	};

}
