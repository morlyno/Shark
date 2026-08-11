#include "skpch.h"
#include "AssetSerializer.h"

#include "Shark/Core/Application.h"

#include "Shark/Serialization/AudioSerializers.h"
#include "Shark/Serialization/EnvironmentSerializer.h"
#include "Shark/Serialization/MaterialSerializer.h"
#include "Shark/Serialization/MeshSerializers.h"
#include "Shark/Serialization/PrefabSerializer.h"
#include "Shark/Serialization/SceneSerializer.h"
#include "Shark/Serialization/SerializerBase.h"
#include "Shark/Serialization/Serializers.h"
#include "Shark/Serialization/TextureSerializer.h"

namespace Shark {

	static std::unordered_map<AssetType, Scope<SerializerBase>> s_Serializers;

	void AssetSerializer::RegisterSerializers()
	{
		s_Serializers[AssetType::Scene]          = Scope<SceneAssetSerializer>::Create();
		s_Serializers[AssetType::Texture]        = Scope<TextureSerializer>::Create();
		s_Serializers[AssetType::ScriptFile]     = Scope<ScriptFileSerializer>::Create();
		s_Serializers[AssetType::Font]           = Scope<FontSerializer>::Create();
		s_Serializers[AssetType::MeshSource]     = Scope<MeshSourceSerializer>::Create();
		s_Serializers[AssetType::Mesh]           = Scope<MeshSerializer>::Create();
		s_Serializers[AssetType::Material]       = Scope<MaterialSerializer>::Create();
		s_Serializers[AssetType::Environment]    = Scope<EnvironmentSerializer>::Create();
		s_Serializers[AssetType::Prefab]         = Scope<PrefabSerializer>::Create();
		s_Serializers[AssetType::AudioFile]      = Scope<AudioFileSerializer>::Create();
		s_Serializers[AssetType::SoundConfig]    = Scope<SoundConfigSerializer>::Create();
		s_Serializers[AssetType::Animation]      = Scope<AnimationSerializer>::Create();
		s_Serializers[AssetType::AnimationGraph] = Scope<AnimationGraphSerializer>::Create();
	}

	void AssetSerializer::ReleaseSerializers()
	{
		s_Serializers.clear();
	}

	void AssetSerializer::RegisterSerializer(AssetType assetType, Scope<SerializerBase> serializer)
	{
		s_Serializers[assetType] = std::move(serializer);
	}

	bool AssetSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		if (s_Serializers.contains(metadata.Type))
		{
			SK_CORE_INFO_TAG("Serialization", "Loading {} from {}", metadata.Type, metadata.FilePath);
			ScopedTimer timer(LogLevel::Info, "Serialization", fmt::format("Loading {} '{}'", metadata.Type, metadata.FilePath));

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
			SK_CORE_INFO_TAG("Serialization", "Serializing {} to {}", metadata.Type, metadata.FilePath);
			ScopedTimer timer(LogLevel::Info, "Serialization", fmt::format("Serializing {} '{}'", metadata.Type, metadata.FilePath));

			const auto& serializer = s_Serializers.at(metadata.Type);
			return serializer->Serialize(asset, metadata);
		}

		SK_CORE_ASSERT(false, "Serializer not found");
		return false;
	}

}

