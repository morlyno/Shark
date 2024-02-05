#include "skpch.h"
#include "MeshSourceSerializer.h"

#include "Shark/Serialization/Import/AssimpMeshImporter.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	bool MeshSourceSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing MeshSource to {}", metadata.FilePath);

		ScopedTimer timer("Serializing MeshSource");
		return true;
	}

	bool MeshSourceSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading MeshSource from {}", metadata.FilePath);

		ScopedTimer timer("Loading MeshSource");

		if (!Project::GetActive()->GetEditorAssetManager()->HasExistingFilePath(metadata))
		{
			SK_CORE_ERROR_TAG("Serialization", "Path not found! {}", metadata.FilePath);
			return false;
		}

		auto filesystemPath = Project::GetActive()->GetEditorAssetManager()->GetFilesystemPath(metadata);
		AssimpMeshImporter importer(filesystemPath);
		Ref<MeshSource> meshSource = importer.ToMeshSourceFromFile();
		if (!meshSource)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to Load MeshSource!");
			return false;
		}

		asset = meshSource;
		asset->Handle = metadata.Handle;
		return true;
	}

}
