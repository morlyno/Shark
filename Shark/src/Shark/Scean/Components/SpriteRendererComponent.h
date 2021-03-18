#pragma once

#include "Shark/Render/Texture.h"

namespace Shark {

	struct SpriteRendererComponent
	{
		SpriteRendererComponent() = default;
		SpriteRendererComponent(const DirectX::XMFLOAT4& color)
			: Color(color) {}
		SpriteRendererComponent(const Ref<Texture2D>& texture)
			: Texture(texture) {}
		SpriteRendererComponent(const DirectX::XMFLOAT4& color, const Ref<Texture2D>& texture, float tilingfactor)
			: Color(color), Texture(texture), TilingFactor(tilingfactor) {}
		~SpriteRendererComponent() = default;

		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<Texture2D> Texture;
		float TilingFactor = 1.0f;
	};

}