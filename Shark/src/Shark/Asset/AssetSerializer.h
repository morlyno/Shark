#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/AssetMetadata.h"

namespace Shark {
	class Asset;
	class SerializerBase;
	class AssetLoadContext;
}

namespace Shark {

	class AssetSerializer
	{
	public:
		static void RegisterSerializers();
		static void ReleaseSerializers();
		static void RegisterSerializer(AssetType assetType, Scope<SerializerBase> serializer);

		static bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context);
		static bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata);
	};

}

