#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/AssetMetadata.h"

#include <filesystem>

namespace Shark {

	// Utility until a permanet solution is provided
	std::filesystem::path GetAssetFilesystemPath(const AssetMetaData& metadata);

}
