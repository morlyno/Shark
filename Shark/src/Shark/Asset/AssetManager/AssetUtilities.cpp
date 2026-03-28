#include "skpch.h"
#include "AssetUtilities.h"

#include "Shark/Core/Project.h"
#include "Shark/File/FileSystem.h"

namespace Shark {

	std::filesystem::path GetAssetFilesystemPath(const AssetMetaData& metadata)
	{
		if (metadata.FilePath.empty())
			return {};

		if (metadata.IsEditorAsset)
			return FileSystem::Absolute(metadata.FilePath);

		return (Project::GetActiveAssetsDirectory() / metadata.FilePath).lexically_normal();
	}

}
