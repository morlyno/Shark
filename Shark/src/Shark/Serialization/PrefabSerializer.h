#pragma once

#include "Shark/Serialization/SerializerBase.h"

namespace Shark {

	class Prefab;

	class PrefabSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context) override;

	private:
		std::string SerializeToYAML(Ref<Prefab> prefab);
		bool DeserializeFromYAML(Ref<Prefab> prefab, const std::string& filedata, AssetLoadContext* context);

	};

}
