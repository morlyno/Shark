#pragma once

#include "Shark/Asset/Asset.h"

namespace Shark {

	class SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) = 0;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) = 0;
		virtual bool Deserialize(Ref<Asset> asset, const std::filesystem::path& assetPath) = 0;
	};

}
