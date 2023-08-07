#pragma once

#include "Shark/Serialization/SerializerBase.h"

namespace Shark {

	class Mesh;

	class MeshSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) override;
		virtual bool Deserialize(Ref<Asset> asset, const std::filesystem::path& assetPath) override;

	private:
		std::string SerializeToYAML(Ref<Mesh> mesh);
		bool DeserializeFromYAML(Ref<Mesh> mesh, const std::string& filedata);

	private:
		std::string m_ErrorMsg;

	};

}
