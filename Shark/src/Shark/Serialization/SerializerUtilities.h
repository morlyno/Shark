#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/AssetTypes.h"

namespace Shark {
	struct AssetMetaData;
	class AssetLoadContext;
}

namespace Shark::Utilities {

	std::filesystem::path GetAssetFilesystemPath(AssetHandle handle);
	std::filesystem::path GetAssetFilesystemPath(const AssetMetaData& metadata);
	bool ValidateYamlAssetFile(const std::filesystem::path& filesystemPath, const AssetMetaData& metadata, AssetLoadContext* context, uint64_t threshold = 16 /*any file smaller than <threshold> bytes is invalid*/);

}
