#include "skpch.h"
#include "MeshSourceSerializer.h"

#include "Shark/File/FileSystem.h"
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

	bool MeshSourceSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading MeshSource from {}", metadata.FilePath);

		ScopedTimer timer("Serialization", "Loading MeshSource");

		auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!FileSystem::Exists(filesystemPath))
		{
			context->OnFileNotFound(metadata);
			return false;
		}

		AssimpMeshImporter importer(filesystemPath);
		Ref<MeshSource> meshSource = importer.ToMeshSourceFromFile(context);

		if (!meshSource)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to Load MeshSource!");
			context->AddError(AssetLoadError::Unknown, "Assimp mesh importer failed to load file");
			return false;
		}

		asset = meshSource;
		asset->Handle = metadata.Handle;
		return true;
	}

}
