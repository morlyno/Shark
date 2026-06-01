#pragma once

#include "Shark/Serialization/SerializerBase.h"

namespace Shark {
	class SoundConfig;
}

namespace Shark {

	class AudioFileSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context) override;
	};

	class SoundConfigSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context) override;

	private:
		std::string SerializeToYAML(Ref<SoundConfig> soundConfig);
		bool DeserializeFromYAML(Ref<SoundConfig> soundConfig, const std::string& filedata, AssetLoadContext* context);
	};

}
