#include "skpch.h"
#include "Physics2DUtils.h"

#include "Physics2DActor.h"
#include <box2d/b2_body.h>

namespace Shark {

	glm::mat4 Physics2DUtils::GetMatrix(Ref<Physics2DActor> actor)
	{
		b2Body* body = actor->GetBody();
		const auto& pos = body->GetPosition();
		return glm::translate(glm::vec3(pos.x, pos.y, 0.0f)) *
			glm::toMat4(glm::quat(glm::vec3(0.0f, 0.0f, body->GetAngle())));
	}

}
