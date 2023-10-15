#include "skpch.h"
#include "MaterialSerializer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/MeshSource.h"
#include "Shark/Asset/ResourceManager.h"

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

		const auto fsPath = ResourceManager::GetFileSystemPath(metadata);
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

		if (!ResourceManager::HasExistingFilePath(metadata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Path not found! {}", metadata.FilePath);
			return false;
		}

		std::string filedata = FileSystem::ReadString(ResourceManager::GetFileSystemPath(metadata));
		if (filedata.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "File was empty");
			return false;
		}

		Ref<MaterialAsset> material = Ref<MaterialAsset>::Create();
		if (!DeserializeFromYAML(material, filedata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to deserialize YAML file!\n\tError Message: {}", m_ErrorMsg);
			return false;
		}

		asset = material;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool MaterialSerializer::Deserialize(Ref<Asset> asset, const std::filesystem::path& assetPath)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Deserializing Material from {}", assetPath);

		ScopedTimer timer("Deserializing Material");
		m_ErrorMsg.clear();

		if (!FileSystem::Exists(assetPath))
		{
			SK_CORE_ERROR_TAG("Serialization", "Path not found! {}", assetPath);
			return false;
		}

		std::string filedata = FileSystem::ReadString(assetPath);
		if (filedata.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "File was empty");
			return false;
		}

		if (!DeserializeFromYAML(asset.As<MaterialAsset>(), filedata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to deserialize YAML file!\n\tError Message: {}", m_ErrorMsg);
			return false;
		}

		return true;
	}

	std::string MaterialSerializer::SerializeToYAML(Ref<MaterialAsset> material)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Material" << YAML::Value;
		out << YAML::BeginMap;

		out << YAML::Key << "AlbedoColor" << YAML::Value << material->GetAlbedoColor();
		out << YAML::Key << "AlbedoTexture" << YAML::Value << material->GetAlbedoTexture();
		out << YAML::Key << "UseAlbedo" << YAML::Value << material->UseAlbedo();
		out << YAML::Key << "Metallic" << YAML::Value << material->GetMetallic();
		out << YAML::Key << "Roughness" << YAML::Value << material->GetRoughness();
		out << YAML::Key << "AO" << YAML::Value << material->GetAO();
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
			AssetHandle albedoTexture = materialNode["AlbedoTexture"].as<AssetHandle>();
			bool useAlbedo = materialNode["UseAlbedo"].as<bool>();

			float metallic = materialNode["Metallic"].as<float>();
			float reoughness = materialNode["Roughness"].as<float>();
			float ao = materialNode["AO"].as<float>();

			material->SetAlbedoColor(albedoColor);
			material->SetAlbedoTexture(albedoTexture);
			material->SetUseAlbedo(useAlbedo);

			material->SetMetallic(metallic);
			material->SetRoughness(reoughness);
			material->SetAO(ao);

			material->UpdateMaterial();
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
