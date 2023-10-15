#include "skpch.h"
#include "MeshSerializer.h"

#include "Shark/Asset/ResourceManager.h"
#include "Shark/Render/Material.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Utils/YAMLUtils.h"

#include <yaml-cpp/yaml.h>

namespace Shark {

	bool MeshSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Mesh to {}", metadata.FilePath);

		Timer timer;
		m_ErrorMsg.clear();

		std::string result = SerializeToYAML(asset.As<Mesh>());
		if (result.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "YAML result was empty!\n\t{}", m_ErrorMsg);
			return false;
		}

		std::ofstream fout(ResourceManager::GetFileSystemPath(metadata));
		SK_CORE_ASSERT(fout);

		fout << result;
		fout.close();

		SK_CORE_INFO_TAG("Serialization", "Serializing Mesh took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

	bool MeshSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_CORE_INFO_TAG("Serialization", "Loading Mesh from {}", metadata.FilePath);

		Timer timer;
		m_ErrorMsg.clear();

		if (!ResourceManager::HasExistingFilePath(metadata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Path not found! {0}", metadata.FilePath);
			return false;
		}

		std::string filedata = FileSystem::ReadString(ResourceManager::GetFileSystemPath(metadata));
		if (filedata.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "File was empty!");
			return false;
		}

		Ref<Mesh> mesh = Ref<Mesh>::Create();
		if (!DeserializeFromYAML(mesh, filedata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to deserialize Mesh from YAML file! {}", m_ErrorMsg);
			return false;
		}

		asset = mesh;
		asset->Handle = metadata.Handle;

		SK_CORE_INFO_TAG("Serialization", "Deserializing Mesh took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

	bool MeshSerializer::Deserialize(Ref<Asset> asset, const std::filesystem::path& assetPath)
	{
		SK_CORE_INFO_TAG("Serialization", "Deserializing Mesh from {}", assetPath);

		Timer timer;
		m_ErrorMsg.clear();

		if (!FileSystem::Exists(assetPath))
		{
			SK_CORE_ERROR_TAG("Serialization", "Path not found! {0}", assetPath);
			return false;
		}

		std::string filedata = FileSystem::ReadString(assetPath);
		if (filedata.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "File was empty!");
			return false;
		}

		if (!DeserializeFromYAML(asset.As<Mesh>(), filedata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to deserialize Mesh from YAML file! {}", m_ErrorMsg);
			return false;
		}

		SK_CORE_INFO_TAG("Serialization", "Deserializing Mesh took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

	std::string MeshSerializer::SerializeToYAML(Ref<Mesh> mesh)
	{
		Ref<MeshSource> meshSource = mesh->GetMeshSource();
		Ref<MaterialTable> materialTable = mesh->GetMaterialTable();

		if (!meshSource || !materialTable)
		{
			m_ErrorMsg = "Invalid Mesh";
			return {};
		}

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Mesh" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "MeshSource" << YAML::Value << mesh->GetMeshSource()->Handle;
		out << YAML::Key << "SubmeshIndices" << YAML::Value << mesh->GetSubmeshIndices();
		out << YAML::BeginMap;
		out << YAML::Key << "Materials" << YAML::Value;
		for (const auto& [index, material] : *mesh->GetMaterialTable())
			out << YAML::Key << index << YAML::Value << material->Handle;
		out << YAML::EndMap;
		out << YAML::EndMap;
		out << YAML::EndMap;

		return out.c_str();
	}

	bool MeshSerializer::DeserializeFromYAML(Ref<Mesh> mesh, const std::string& filedata)
	{
		YAML::Node in = YAML::Load(filedata);
		if (!in)
		{
			m_ErrorMsg = "Failed to load YAML!";
			return false;
		}

		auto meshNode = in["Mesh"];
		if (!meshNode)
		{
			m_ErrorMsg = "Mesh Node not found!";
			return false;
		}

		auto meshSource = meshNode["MeshSource"].as<AssetHandle>();
		auto subMeshes = meshNode["SubmeshIndices"].as<std::vector<uint32_t>>();
		auto materials = meshNode["Materials"];

		mesh->m_MeshSource = ResourceManager::GetAsset<MeshSource>(meshSource);
		mesh->m_MaterialTable = Ref<MaterialTable>::Create();
		for (const auto& element : materials)
		{
			uint32_t index = element.first.as<uint32_t>();
			AssetHandle handle = element.second.as<AssetHandle>();

			Ref<MaterialAsset> material = ResourceManager::GetAsset<MaterialAsset>(handle);
			mesh->m_MaterialTable->AddMaterial(index, material);
		}

		mesh->m_SubmeshIndices = std::move(subMeshes);

		return true;
	}

}
