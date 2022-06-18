#include "skpch.h"
#include "AssetSerializer.h"

#include "Shark/Asset/ResourceManager.h"

#include "Shark/Asset/SceneSerialization.h"
#include "Shark/Asset/TextureSerialization.h"

namespace Shark {

	static std::unordered_map<AssetType, Scope<Serializer>> s_Serializers;

	void AssetSerializer::RegisterSerializers()
	{
		s_Serializers[AssetType::None] = nullptr;
		s_Serializers[AssetType::Scene] = Scope<SceneSerializer>::Create();
		s_Serializers[AssetType::Texture] = Scope<TextureSerializer>::Create();
	}

	void AssetSerializer::ReleaseSerializers()
	{
		s_Serializers.clear();
	}

	bool AssetSerializer::TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		const auto& serializer = s_Serializers.at(metadata.Type);
		if (serializer)
			return serializer->TryLoadData(asset, metadata);

		SK_CORE_ASSERT(false, "Serializer was null");
		return false;
	}

	bool AssetSerializer::Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		const auto& serializer = s_Serializers.at(metadata.Type);
		if (serializer)
			return serializer->Serialize(asset, metadata);

		SK_CORE_ASSERT(false, "Serializer was null");
		return false;
	}

}

