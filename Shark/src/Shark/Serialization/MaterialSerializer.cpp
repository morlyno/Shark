#include "skpch.h"
#include "MaterialSerializer.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Asset/AssetManager/AssetUtilities.h"
#include "Shark/Render/MaterialAsset.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/YAML.h"
#include "Shark/Serialization/SerializationMacros.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	bool MaterialSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Material to {}", metadata.FilePath);

		ScopedTimer timer("Serializing Material");

		std::string result = SerializeToYAML(asset.As<PBRMaterial>());
		if (result.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "YAML result was empty!");
			return false;
		}

		const auto filesystemPath = GetAssetFilesystemPath(metadata);
		FileSystem::WriteString(filesystemPath, result);

		return true;
	}

	bool MaterialSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading Material from {}", metadata.FilePath);

		ScopedTimer timer("Loading Material");

		const auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!FileSystem::Exists(filesystemPath))
		{
			context->OnFileNotFound(metadata);
			return false;
		}

		std::string filedata = FileSystem::ReadString(filesystemPath);
		if (filedata.empty())
		{
			context->OnFileEmpty(metadata);
			return false;
		}

		Ref<PBRMaterial> material = PBRMaterial::Create(FileSystem::GetStemString(metadata.FilePath), true, false);
		if (!DeserializeFromYAML(material, filedata, context))
		{
			context->OnYamlError(metadata);
			return false;
		}

		//
		// no longer necessary to call bake
		// if bake becomes necessary again call either
		// 
		//	context->AddTask([material]() { material->MT_Bake(); });
		// or
		//	material->MT_Bake();
		//

		asset = material;
		asset->Handle = metadata.Handle;
		context->SetStatus(AssetLoadStatus::Ready);
		return true;
	}

	std::string MaterialSerializer::SerializeToYAML(Ref<PBRMaterial> material)
	{
		SK_PROFILE_FUNCTION();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Material" << YAML::Value;
		out << YAML::BeginMap;

		out << YAML::Key << "AlbedoColor" << YAML::Value << material->GetAlbedoColor();
		out << YAML::Key << "Metallic" << YAML::Value << material->GetMetalness();
		out << YAML::Key << "Roughness" << YAML::Value << material->GetRoughness();

		out << YAML::Key << "AlbedoMap" << YAML::Value << material->GetAlbedoMap();
		out << YAML::Key << "UsingNormalMap" << YAML::Value << material->IsUsingNormalMap();
		out << YAML::Key << "NormalMap" << YAML::Value << material->GetNormalMap();
		out << YAML::Key << "MetalnessMap" << YAML::Value << material->GetMetalnessMap();
		out << YAML::Key << "RoughnessMap" << YAML::Value << material->GetRoughnessMap();
		out << YAML::EndMap;
		out << YAML::EndMap;
		
		return out.c_str();
	}

	bool MaterialSerializer::DeserializeFromYAML(Ref<PBRMaterial> material, const std::string& filedata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		auto rootNode = YAML::Load(filedata);
		if (!rootNode)
			return false;

		auto materialNode = rootNode["Material"];
		if (!materialNode)
		{
			context->AddError(AssetLoadError::InvalidYAML, "Root node 'Material' missing");
			return false;
		}

		SK_DESERIALIZE_PROPERTY(materialNode, "AlbedoColor", material->GetAlbedoColor());
		SK_DESERIALIZE_PROPERTY(materialNode, "Metallic", material->GetMetalness());
		SK_DESERIALIZE_PROPERTY(materialNode, "Roughness", material->GetRoughness());

		bool usingNormalMap = false;
		SK_DESERIALIZE_PROPERTY(materialNode, "UsingNormalMap", usingNormalMap, true);

		AssetHandle albedoMap, normalMap, metalnessMap, roughnessMap;
		SK_DESERIALIZE_PROPERTY(materialNode, "AlbedoMap", albedoMap, AssetHandle::Invalid);
		SK_DESERIALIZE_PROPERTY(materialNode, "NormalMap", normalMap, AssetHandle::Invalid);
		SK_DESERIALIZE_PROPERTY(materialNode, "MetalnessMap", metalnessMap, AssetHandle::Invalid);
		SK_DESERIALIZE_PROPERTY(materialNode, "RoughnessMap", roughnessMap, AssetHandle::Invalid);

		material->SetUsingNormalMap(usingNormalMap && normalMap);
		if (albedoMap)    material->SetAlbedoMap(albedoMap);
		if (normalMap)    material->SetNormalMap(normalMap);
		if (metalnessMap) material->SetMetalnessMap(metalnessMap);
		if (roughnessMap) material->SetRoughnessMap(roughnessMap);

		context->AddTask([material](AssetLoadContext* context)
		{
			auto albedo = material->GetAlbedoMap();
			auto normal = material->GetNormalMap();
			auto metalness = material->GetMetalnessMap();
			auto roughness = material->GetRoughnessMap();

			if (AssetManager::GetAssetType(albedo) != AssetType::Texture)
				material->ClearAlbedoMap();

			if (AssetManager::GetAssetType(normal) != AssetType::Texture)
				material->ClearNormalMap(true);

			if (AssetManager::GetAssetType(metalness) != AssetType::Texture)
				material->ClearMetalnessMap();

			if (AssetManager::GetAssetType(roughness) != AssetType::Texture)
				material->ClearRoughnessMap();

			context->SetStatus(AssetLoadStatus::Ready);
		});

		return true;
	}

}
