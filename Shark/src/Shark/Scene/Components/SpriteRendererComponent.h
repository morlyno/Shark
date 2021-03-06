#pragma once

#include "Shark/Render/Texture.h"
#include "Shark/Render/Material.h"

namespace Shark {

	struct SpriteRendererComponent
	{
		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<Texture2D> Texture = nullptr;
		float TilingFactor = 1.0f;
		Ref<Material> Material;
	};

}
