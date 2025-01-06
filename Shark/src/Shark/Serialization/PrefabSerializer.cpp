#include "skpch.h"
#include "PrefabSerializer.h"

#include "Shark/Scene/Prefab.h"
#include "Shark/Serialization/YAML.h"
#include "Shark/Serialization/SceneSerializer.h"
#include "Shark/Serialization/SerializationMacros.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	bool PrefabSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing Prefab to {}", metadata.FilePath);
		ScopedTimer timer("Serializing Prefab");

		std::string result = SerializeToYAML(asset.As<Prefab>());
		if (result.empty())
		{
			SK_CORE_ERROR_TAG("Serialization", "YAML result was empty!");
			return false;
		}

		return FileSystem::WriteString(Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata), result);
	}

	bool PrefabSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Deserializing Prefab from {}", metadata.FilePath);
		ScopedTimer timer("Loading Prefab");

		if (!Project::GetActiveEditorAssetManager()->HasExistingFilePath(metadata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Path not found! {0}", metadata.FilePath);
			return false;
		}

		const std::string filedata = FileSystem::ReadString(Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata));

		Ref<Prefab> prefab = Ref<Prefab>::Create();
		if (!DeserializeFromYAML(prefab, filedata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to deserialize Prefab from YAML");
			return false;
		}

		asset = prefab;
		asset->Handle = metadata.Handle;
		return true;
	}

	std::string PrefabSerializer::SerializeToYAML(Ref<Prefab> prefab)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Prefab";
		out << YAML::BeginMap;
		out << YAML::Key << "RootID" << YAML::Value << prefab->m_RootEntityID;
		out << YAML::Key << "ActiveCameraID" << YAML::Value << prefab->GetActiveCameraID();
		out << YAML::Key << "SetActiveCamera" << YAML::Value << prefab->m_SetActiveCamera;
		out << YAML::Key << "Entities" << YAML::Value;
		out << YAML::BeginSeq;

		Ref<Scene> prefabScene = prefab->GetScene();
		SceneSerializer serializer(prefabScene);

		std::vector<Entity> entities = prefabScene->GetEntitiesSorted();
		for (Entity entity : entities)
		{
			serializer.SerializeEntity(out, entity);
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;
		out << YAML::EndMap;
		return out.c_str();
	}

	bool PrefabSerializer::DeserializeFromYAML(Ref<Prefab> prefab, const std::string& filedata)
	{
		YAML::Node rootNode = YAML::Load(filedata);
		if (!rootNode)
			return false;

		YAML::Node prefabNode = rootNode["Prefab"];
		if (!prefabNode)
			return false;

		UUID activeCameraID;

		SK_DESERIALIZE_PROPERTY(prefabNode, "RootID", prefab->m_RootEntityID, UUID::Invalid);
		SK_DESERIALIZE_PROPERTY(prefabNode, "ActiveCameraID", activeCameraID, UUID::Invalid);
		SK_DESERIALIZE_PROPERTY(prefabNode, "SetActiveCamera", prefab->m_SetActiveCamera, UUID::Invalid);

		prefab->m_Scene->SetActiveCamera(activeCameraID);

		SceneSerializer serializer(prefab->m_Scene);

		YAML::Node entitiesNode = prefabNode["Entities"];
		for (auto entityNode : entitiesNode)
		{
			serializer.DeserializeEntity(entityNode);
		}

		return true;
	}

}
