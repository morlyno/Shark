#pragma once

#include "Shark/Render/Texture.h"

namespace Shark {

	enum class Geometry
	{
		None = 0,
		Quad, Circle
		// TODO: Polygon
	};

	struct SpriteRendererComponent
	{
		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<Texture2D> Texture = nullptr;
		float TilingFactor = 1.0f;
		Geometry Geometry = Geometry::Quad;
	};

}
