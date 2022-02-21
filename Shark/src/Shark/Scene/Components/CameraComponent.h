#pragma once

#include "Shark/Scene/SceneCamera.h"

namespace Shark{

	struct CameraComponent
	{
		SceneCamera Camera;
		const glm::mat4& GetProjection() const { return Camera.GetProjection(); }
	};

}