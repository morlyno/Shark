#include "skpch.h"
#include "AssetSerializer.h"

#include "Shark/Core/Application.h"
#include "Shark/Asset/AssetUtils.h"

#include "Shark/Serialization/SerializerBase.h"
#include "Shark/Serialization/SceneSerializer.h"
#include "Shark/Serialization/TextureSerializer.h"
#include "Shark/Serialization/Serializers.h"
#include "Shark/Serialization/MeshSourceSerializer.h"
#include "Shark/Serialization/MeshSerializer.h"
#include "Shark/Serialization/MaterialSerializer.h"
#include "Shark/Serialization/EnvironmentSerializer.h"
#include "Shark/Serialization/PrefabSerializer.h"

namespace Shark {

	static std::unordered_map<AssetType, Scope<SerializerBase>> s_Serializers;

	void AssetSerializer::RegisterSerializers()
	{
		s_Serializers[AssetType::Scene] = Scope<SceneAssetSerializer>::Create();
		s_Serializers[AssetType::Texture] = Scope<TextureSerializer>::Create();
		s_Serializers[AssetType::ScriptFile] = Scope<ScriptFileSerializer>::Create();
		s_Serializers[AssetType::Font] = Scope<FontSerializer>::Create();
		s_Serializers[AssetType::MeshSource] = Scope<MeshSourceSerializer>::Create();
		s_Serializers[AssetType::Mesh] = Scope<MeshSerializer>::Create();
		s_Serializers[AssetType::Material] = Scope<MaterialSerializer>::Create();
		s_Serializers[AssetType::Environment] = Scope<EnvironmentSerializer>::Create();
		s_Serializers[AssetType::Prefab] = Scope<PrefabSerializer>::Create();
	}

	void AssetSerializer::ReleaseSerializers()
	{
		s_Serializers.clear();
	}

	bool AssetSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		if (s_Serializers.contains(metadata.Type))
		{
			const auto& serializer = s_Serializers.at(metadata.Type);
			return serializer->TryLoadAsset(asset, metadata, context);
		}

		SK_CORE_ASSERT(false, "Serializer not found");
		return false;
	}

	bool AssetSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_CORE_VERIFY(Application::IsMainThread(), "AssetSerializer::Serialize can only be called from the main thread");
		if (s_Serializers.contains(metadata.Type))
		{
			const auto& serializer = s_Serializers.at(metadata.Type);
			return serializer->Serialize(asset, metadata);
		}

		SK_CORE_ASSERT(false, "Serializer not found");
		return false;
	}

}

