#include "skpch.h"
#include "MaterialSerializer.h"

#include "Shark/Asset/AssetManager.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Render/MeshSource.h"

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
		m_ErrorMsg.clear();

		std::string result = SerializeToYAML(asset.As<PBRMaterial>());
		if (result.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "YAML result was empty!\n\tError Message: {}", m_ErrorMsg);
			return false;
		}

		const auto fsPath = Project::GetEditorAssetManager()->GetFilesystemPath(metadata);
		FileSystem::WriteString(fsPath, result);

		return true;
	}

	bool MaterialSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading Material from {}", metadata.FilePath);

		ScopedTimer timer("Loading Material");
		m_ErrorMsg.clear();

		if (!Project::GetEditorAssetManager()->HasExistingFilePath(metadata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Path not found! {}", metadata.FilePath);
			return false;
		}

		std::string filedata = FileSystem::ReadString(Project::GetEditorAssetManager()->GetFilesystemPath(metadata));
		if (filedata.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "File was empty");
			return false;
		}

		Ref<PBRMaterial> material = PBRMaterial::Create(FileSystem::GetStemString(metadata.FilePath));
		if (!DeserializeFromYAML(material, filedata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to deserialize YAML file!\n\tError Message: {}", m_ErrorMsg);
			return false;
		}

		asset = material;
		asset->Handle = metadata.Handle;
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
		m_ErrorMsg = out.GetLastError();
		return out.c_str();
	}

	bool MaterialSerializer::DeserializeFromYAML(Ref<PBRMaterial> material, const std::string& filedata)
	{
		SK_PROFILE_FUNCTION();

		auto rootNode = YAML::Load(filedata);
		if (!rootNode)
		{
			m_ErrorMsg = "Failed to load YAML";
			return false;
		}

		auto materialNode = rootNode["Material"];
		if (!materialNode)
		{
			m_ErrorMsg = "Material Node not found";
			return false;
		}

		SK_DESERIALIZE_PROPERTY(materialNode, "AlbedoColor", material->GetAlbedoColor(), glm::vec3(1.0f));
		SK_DESERIALIZE_PROPERTY(materialNode, "Metallic", material->GetMetalness(), 0.0f);
		SK_DESERIALIZE_PROPERTY(materialNode, "Roughness", material->GetRoughness(), 0.5f);

		bool usingNormalMap;
		SK_DESERIALIZE_PROPERTY(materialNode, "UsingNormalMap", usingNormalMap, false);

		AssetHandle albedoMap, normalMap, metalnessMap, roughnessMap;
		SK_DESERIALIZE_PROPERTY(materialNode, "AlbedoMap", albedoMap, AssetHandle::Invalid);
		SK_DESERIALIZE_PROPERTY(materialNode, "NormalMap", normalMap, AssetHandle::Invalid);
		SK_DESERIALIZE_PROPERTY(materialNode, "MetalnessMap", metalnessMap, AssetHandle::Invalid);
		SK_DESERIALIZE_PROPERTY(materialNode, "RoughnessMap", roughnessMap, AssetHandle::Invalid);

		if (AssetManager::IsValidAssetHandle(albedoMap) && AssetManager::GetAssetType(albedoMap) == AssetType::Texture)
		{
			material->SetAlbedoMap(albedoMap);
		}

		if (AssetManager::IsValidAssetHandle(normalMap) && AssetManager::GetAssetType(normalMap) == AssetType::Texture)
		{
			material->SetUsingNormalMap(usingNormalMap);
			material->SetNormalMap(normalMap);
		}

		if (AssetManager::IsValidAssetHandle(metalnessMap) && AssetManager::GetAssetType(metalnessMap) == AssetType::Texture)
		{
			material->SetMetalnessMap(metalnessMap);
		}

		if (AssetManager::IsValidAssetHandle(roughnessMap) && AssetManager::GetAssetType(roughnessMap) == AssetType::Texture)
		{
			material->SetRoughnessMap(roughnessMap);
		}

		return true;
	}

}
