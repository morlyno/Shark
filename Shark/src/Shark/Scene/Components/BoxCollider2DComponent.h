#pragma once

class b2Fixture;

namespace Shark {

	struct BoxCollider2DComponent
	{
		DirectX::XMFLOAT2 Size = { 0.5f, 0.5f };
		DirectX::XMFLOAT2 Offset = { 0.0f, 0.0f };
		float Rotation = 0.0f;
		
		float Density = 1.0f;
		float Friction = 0.2f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 1.0f;

		b2Fixture* RuntimeCollider;

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
	};

}
