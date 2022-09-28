#include "skpch.h"
#include "AssetSerializer.h"

#include "Shark/Asset/Serializers.h"

namespace Shark {

	static std::unordered_map<AssetType, Scope<Serializer>> s_Serializers;

	void AssetSerializer::RegisterSerializers()
	{
		s_Serializers[AssetType::Scene] = Scope<SceneSerializer>::Create();
		s_Serializers[AssetType::Texture] = Scope<TextureSerializer>::Create();
		s_Serializers[AssetType::TextureSource] = Scope<TextureSourceSerializer>::Create();
		s_Serializers[AssetType::ScriptFile] = Scope<ScriptFileSerializer>::Create();
	}

	void AssetSerializer::ReleaseSerializers()
	{
		s_Serializers.clear();
	}

	bool AssetSerializer::TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		if (s_Serializers.find(metadata.Type) != s_Serializers.end())
		{
			const auto& serializer = s_Serializers.at(metadata.Type);
			return serializer->TryLoadData(asset, metadata);
		}

		SK_CORE_ASSERT(false, "Serializer not found");
		return false;
	}

	bool AssetSerializer::Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		if (s_Serializers.find(metadata.Type) != s_Serializers.end())
		{
			const auto& serializer = s_Serializers.at(metadata.Type);
			return serializer->Serialize(asset, metadata);
		}

		SK_CORE_ASSERT(false, "Serializer not found");
		return false;
	}

}

