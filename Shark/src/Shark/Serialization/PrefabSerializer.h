#pragma once

#include "Shark/Serialization/SerializerBase.h"
#include "Shark/Scene/Prefab.h"

namespace Shark {

	class PrefabSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) override;

	private:
		std::string SerializeToYAML(Ref<Prefab> prefab);
		bool DeserializeFromYAML(Ref<Prefab> prefab, const std::string& filedata);

	};

}
