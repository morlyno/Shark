#pragma once

#include <DirectXMath.h>

class b2Fixture;

namespace Shark {

	struct CircleCollider2DComponent
	{
		float Radius = 0.5f;
		DirectX::XMFLOAT2 LocalOffset = { 0.0f, 0.0f };
		float LocalRotation = 0.0f;

		float Density = 0.0f;
		float Friction = 0.2f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 1.0f;

		b2Fixture* RuntimeCollider;

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
	};

}