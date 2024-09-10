#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Asset/AssetMetadata.h"
#include "Shark/Serialization/SerializerBase.h"

namespace Shark {

	class AssetSerializer
	{
	public:
		static void RegisterSerializers();
		static void ReleaseSerializers();

		static bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata);
		static bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata);

		static SerializerBase* GetSerializer(AssetType type);

		template<typename TSerializer>
		static TSerializer* GetSerializer()
		{
			return (TSerializer*)GetSerializer(TSerializer::GetAssetType());
		}

	};

}

