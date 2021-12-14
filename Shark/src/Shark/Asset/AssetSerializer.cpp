#include "skpch.h"
#include "AssetSerializer.h"

#include "Shark/Asset/ResourceManager.h"

#include "Shark/Asset/SceneSerialization.h"
#include "Shark/Asset/TextureSerialization.h"

namespace Shark {

	static std::unordered_map<AssetType, Ref<SerializerBase>> s_Serializers = {
		{ AssetType::Scene, Ref<SceneSerializer>::Create() },
		{ AssetType::Texture, Ref<TextureSerializer>::Create() }
	};

	bool AssetSerializer::TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		if (metadata.Type == AssetType::None)
			return false;

		auto serializer = s_Serializers.at(metadata.Type);
		return serializer->TryLoadData(asset, ResourceManager::GetFileSystemPath(metadata));
	}

	bool AssetSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		if (metadata.Type == AssetType::None)
			return false;

		auto serializer = s_Serializers.at(metadata.Type);
		return serializer->Serialize(asset, ResourceManager::GetFileSystemPath(metadata));
	}

	bool AssetSerializer::Deserialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		if (metadata.Type == AssetType::None)
			return false;

		auto serializer = s_Serializers.at(metadata.Type);
		return serializer->Deserialize(asset, ResourceManager::GetFileSystemPath(metadata));
	}

}

