#include "skpch.h"
#include "MeshSerializer.h"

#include "Shark/Core/Project.h"
#include "Shark/Asset/AssetManager.h"

#include "Shark/Render/Mesh.h"
#include "Shark/Render/MeshSource.h"
#include "Shark/Render/Material.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Utils/YAMLUtils.h"
#include "Shark/Debug/Profiler.h"

#include <yaml-cpp/yaml.h>

namespace Shark {

	bool MeshSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Mesh to {}", metadata.FilePath);

		ScopedTimer timer("Serializing Mesh");
		m_ErrorMsg.clear();

		std::string result = SerializeToYAML(asset.As<Mesh>());
		if (result.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "YAML result was empty!\n\t{}", m_ErrorMsg);
			return false;
		}

		const bool success = FileSystem::WriteString(Project::GetActive()->GetEditorAssetManager()->GetFilesystemPath(metadata), result);
		return success;
	}

	bool MeshSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading Mesh from {}", metadata.FilePath);

		ScopedTimer timer("Loading Mesh");
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
		return true;
	}

	std::string MeshSerializer::SerializeToYAML(Ref<Mesh> mesh)
	{
		SK_PROFILE_FUNCTION();

		if (!AssetManager::IsValidAssetHandle(mesh->GetMeshSource()))
		{
			m_ErrorMsg = "Invalid MeshSource Handle";
			return {};
		}

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Mesh" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "MeshSource" << YAML::Value << mesh->GetMeshSource();
		out << YAML::Key << "Submeshes" << YAML::Value << mesh->GetSubmeshes();
		out << YAML::EndMap;
		out << YAML::EndMap;

		return out.c_str();
	}

	bool MeshSerializer::DeserializeFromYAML(Ref<Mesh> mesh, const std::string& filedata)
	{
		SK_PROFILE_FUNCTION();

		YAML::Node in = YAML::Load(filedata);
		if (!in)
		{
			m_ErrorMsg = "Invalid YAML file!";
			return false;
		}

		auto meshNode = in["Mesh"];
		if (!meshNode)
		{
			m_ErrorMsg = "Mesh Node not found!";
			return false;
		}

		mesh->m_MeshSource = meshNode["MeshSource"].as<AssetHandle>();
		mesh->m_Submeshes = meshNode["Submeshes"].as<std::vector<uint32_t>>();
		return true;
	}

}
