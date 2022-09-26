#pragma once

#include "Shark/Asset/Asset.h"

namespace Shark {

	class TextureSource : public Asset
	{
	public:
		TextureSource() = default;
		virtual ~TextureSource() = default;

		static AssetType GetStaticType() { return AssetType::TextureSource; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		static Ref<TextureSource> Create() { return Ref<TextureSource>::Create(); }
	};

}
