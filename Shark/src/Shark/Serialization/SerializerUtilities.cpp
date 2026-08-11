#include "skpch.h"
#include "SerializerUtilities.h"

#include "Shark/Core/Project.h"
#include "Shark/Asset/AssetThread/AssetLoadContext.h"
#include "Shark/File/FileSystem.h"

namespace Shark::Utilities {

	std::filesystem::path GetAssetFilesystemPath(const AssetMetaData& metadata)
	{
		if (metadata.FilePath.empty())
			return {};

		if (metadata.IsEditorAsset)
			return FileSystem::Absolute(metadata.FilePath);

		return (Project::GetActiveAssetsDirectory() / metadata.FilePath).lexically_normal();
	}

	bool ValidateYamlAssetFile(const std::filesystem::path& filesystemPath, const AssetMetaData& metadata, AssetLoadContext* context, uint64_t threshold)
	{
		if (!FileSystem::Exists(filesystemPath))
		{
			context->OnFileNotFound(metadata);
			return false;
		}

		std::error_code error;
		const auto filesize = std::filesystem::file_size(filesystemPath, error);
		if (error || filesize < threshold)
		{
			context->OnFileEmpty(metadata);
			return false;
		}

		return true;
	}

}
