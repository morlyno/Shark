#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Asset/AssetMetadata.h"

namespace Shark {

	class SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) = 0;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) = 0;
	};

}
