#include "skpch.h"
#include "MaterialSerializer.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Render/MeshSource.h"

#include "Shark/Utils/YAMLUtils.h"
#include "Shark/Debug/Profiler.h"

#include <yaml-cpp/yaml.h>

namespace Shark {

	bool MaterialSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Material to {}", metadata.FilePath);

		Timer timer;
		m_ErrorMsg.clear();

		std::string result = SerializeToYAML(asset.As<MaterialAsset>());
		if (result.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "YAML result was empty!\n\tError Message: {}", m_ErrorMsg);
			return false;
		}

		const auto fsPath = Project::GetActive()->GetEditorAssetManager()->GetFilesystemPath(metadata);
		FileSystem::WriteString(fsPath, result);

		SK_CORE_TRACE_TAG("Serialization", "Serializing Material took {}", timer.Elapsed());
		return true;
	}

	bool MaterialSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading Material from {}", metadata.FilePath);

		ScopedTimer timer("Loading Material");
		m_ErrorMsg.clear();

		if (!Project::GetActiveEditorAssetManager()->HasExistingFilePath(metadata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Path not found! {}", metadata.FilePath);
			return false;
		}

		std::string filedata = FileSystem::ReadString(Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata));
		if (filedata.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "File was empty");
			return false;
		}

		Ref<MaterialAsset> material = Ref<MaterialAsset>::Create(Material::Create(Renderer::GetShaderLibrary()->Get("SharkPBR"), FileSystem::GetStemString(metadata.FilePath)));
		if (!DeserializeFromYAML(material, filedata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to deserialize YAML file!\n\tError Message: {}", m_ErrorMsg);
			return false;
		}

		asset = material;
		asset->Handle = metadata.Handle;
		return true;
	}

	std::string MaterialSerializer::SerializeToYAML(Ref<MaterialAsset> material)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Material" << YAML::Value;
		out << YAML::BeginMap;

		out << YAML::Key << "AlbedoColor" << YAML::Value << material->GetAlbedoColor();
		out << YAML::Key << "Metallic" << YAML::Value << material->GetMetalness();
		out << YAML::Key << "Roughness" << YAML::Value << material->GetRoughness();
		out << YAML::Key << "AmbientOcclusion" << YAML::Value << material->GetAmbientOcclusion();

		out << YAML::Key << "AlbedoMap" << YAML::Value << material->GetAlbedoMap()->Handle;
		out << YAML::Key << "UsingNormalMap" << YAML::Value << material->IsUsingNormalMap();
		out << YAML::Key << "NormalMap" << YAML::Value << material->GetNormalMap()->Handle;
		out << YAML::Key << "MetalnessMap" << YAML::Value << material->GetMetalnessMap()->Handle;
		out << YAML::Key << "RoughnessMap" << YAML::Value << material->GetRoughnessMap()->Handle;
		out << YAML::EndMap;
		out << YAML::EndMap;
		m_ErrorMsg = out.GetLastError();
		return out.c_str();
	}

	bool MaterialSerializer::DeserializeFromYAML(Ref<MaterialAsset> material, const std::string& filedata)
	{
		try
		{
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

			glm::vec3 albedoColor = materialNode["AlbedoColor"].as<glm::vec3>();
			float metallic = materialNode["Metallic"].as<float>();
			float reoughness = materialNode["Roughness"].as<float>();
			float ao = materialNode["AmbientOcclusion"].as<float>();

			AssetHandle albedoMap = materialNode["AlbedoMap"].as<AssetHandle>();

			bool usingNormalMap = materialNode["UsingNormalMap"].as<bool>();
			AssetHandle normalMap = materialNode["NormalMap"].as<AssetHandle>();
			AssetHandle metalnessMap = materialNode["MetalnessMap"].as<AssetHandle>(AssetHandle::Invalid);
			AssetHandle roughnessMap = materialNode["RoughnessMap"].as<AssetHandle>(AssetHandle::Invalid);

			material->SetAlbedoColor(albedoColor);
			material->SetMetalness(metallic);
			material->SetRoughness(reoughness);
			material->SetAmbientOcclusion(ao);

			if (AssetManager::IsValidAssetHandle(albedoMap))
			{
				material->SetAlbedoMap(AssetManager::GetAsset<Texture2D>(albedoMap));
			}

			if (AssetManager::IsValidAssetHandle(normalMap))
			{
				material->SetUsingNormalMap(usingNormalMap);
				material->SetNormalMap(AssetManager::GetAsset<Texture2D>(normalMap));
			}

			if (AssetManager::IsValidAssetHandle(metalnessMap))
			{
				material->SetMetalnessMap(AssetManager::GetAsset<Texture2D>(metalnessMap));
			}

			if (AssetManager::IsValidAssetHandle(roughnessMap))
			{
				material->SetRoughnessMap(AssetManager::GetAsset<Texture2D>(roughnessMap));
			}

		}
		catch (const YAML::Exception& exception)
		{
			m_ErrorMsg = exception.what();
			SK_CORE_ASSERT(false);
			return false;
		}
		return true;
	}

}
