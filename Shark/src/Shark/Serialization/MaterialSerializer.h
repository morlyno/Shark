#pragma once

#include "Shark/Serialization/SerializerBase.h"

namespace Shark {

	class PBRMaterial;

	class MaterialSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) override;

	private:
		std::string SerializeToYAML(Ref<PBRMaterial> material);
		bool DeserializeFromYAML(Ref<PBRMaterial> material, const std::string& filedata);

	private:
		std::string m_ErrorMsg;
	};

}
