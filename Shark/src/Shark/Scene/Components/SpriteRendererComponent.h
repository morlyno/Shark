#pragma once

#include "Shark/Render/Texture.h"

namespace Shark {

	struct SpriteRendererComponent
	{
		enum class GeometryType
		{
			None = 0,
			Quad, Circle
			// TODO: Polygon
		};

		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<Texture2D> Texture = nullptr;
		float TilingFactor = 1.0f;
		GeometryType Geometry = GeometryType::Quad;
	};

}
