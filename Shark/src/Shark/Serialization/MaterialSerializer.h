#pragma once

#include "Shark/Serialization/SerializerBase.h"

namespace Shark {

	class MaterialAsset;

	class MaterialSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) override;
		virtual bool Deserialize(Ref<Asset> asset, const std::filesystem::path& assetPath) override;

	private:
		std::string SerializeToYAML(Ref<MaterialAsset> material);
		bool DeserializeFromYAML(Ref<MaterialAsset> material, const std::string& filedata);

	private:
		std::string m_ErrorMsg;
	};

}
