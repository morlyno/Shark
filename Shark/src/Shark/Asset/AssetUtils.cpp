#include "skpch.h"
#include "AssetUtils.h"

#include "Shark/Asset/AssetTypes.h"

namespace Shark {

	AssetType AssetUtils::GetAssetTypeFromPath(const std::filesystem::path& assetPath)
	{
		std::string extension = assetPath.extension().string();
		if (AssetExtensionMap.contains(extension))
			return AssetExtensionMap.at(extension);
		return AssetType::None;
	}

}
