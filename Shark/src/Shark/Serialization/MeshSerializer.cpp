#include "skpch.h"
#include "MeshSerializer.h"

#include "Shark/Core/Project.h"

#include "Shark/Render/Mesh.h"
#include "Shark/Render/MeshSource.h"
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

		std::ofstream fout(Project::GetActive()->GetEditorAssetManager()->GetFilesystemPath(metadata));
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

		if (!Project::GetActive()->GetEditorAssetManager()->HasExistingFilePath(metadata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Path not found! {0}", metadata.FilePath);
			return false;
		}

		std::string filedata = FileSystem::ReadString(Project::GetActive()->GetEditorAssetManager()->GetFilesystemPath(metadata));
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
		out << YAML::Key << "Submeshes" << YAML::Value << mesh->GetSubmeshes();
		out << YAML::Key << "Materials" << YAML::Value << mesh->GetMaterialTable()->GetMaterials();
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
		auto subMeshes = meshNode["Submeshes"].as<std::vector<uint32_t>>();
		auto materials = meshNode["Materials"].as<MaterialTable::MaterialMap>();

		mesh->m_MeshSource = AssetManager::GetAsset<MeshSource>(meshSource);
		mesh->m_Submeshes = std::move(subMeshes);
		mesh->m_MaterialTable = Ref<MaterialTable>::Create();
		mesh->m_MaterialTable->GetMaterials() = std::move(materials);

		return true;
	}

}

namespace YAML {

	template<>
	struct convert<Shark::Ref<Shark::MaterialAsset>>
	{
		static Node encode(Shark::Ref<Shark::MaterialAsset> material)
		{
			return convert<Shark::AssetHandle>::encode(material->Handle);
		}

		static bool decode(Node node, Shark::Ref<Shark::MaterialAsset>& outMaterial)
		{
			Shark::AssetHandle handle;
			if (!convert<Shark::AssetHandle>::decode(node, handle))
				return false;

			outMaterial = Shark::AssetManager::GetAsset<Shark::MaterialAsset>(handle);
			return true;
		}
	};

}
