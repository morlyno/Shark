#pragma once

#include "Shark/Scean/SceanCamera.h"

namespace Shark{

	struct CameraComponent
	{
		CameraComponent() = default;
		CameraComponent(const Camera& camera)
			: Camera(camera) {}
		CameraComponent(const SceanCamera& camera)
			: Camera(camera) {}
		CameraComponent(const DirectX::XMMATRIX& projection)
			: Camera(projection) {}
		~CameraComponent() = default;

		SceanCamera Camera;
	};

}