#include "skpch.h"
#include "AssetManager.h"

#include "Shark/Core/Project.h"

namespace Shark {

	Ref<Asset> AssetManager::GetAsset(AssetHandle handle)
	{
		return Project::GetActive()->GetAssetManager()->GetAsset(handle);
	}

	AssetHandle AssetManager::AddMemoryAsset(Ref<Asset> asset)
	{
		return Project::GetActive()->GetAssetManager()->AddMemoryAsset(asset);
	}

	AssetType AssetManager::GetAssetType(AssetHandle handle)
	{
		return Project::GetActive()->GetAssetManager()->GetAssetType(handle);
	}

	bool AssetManager::IsMemoryAsset(AssetHandle handle)
	{
		return Project::GetActive()->GetAssetManager()->IsMemoryAsset(handle);
	}

	bool AssetManager::IsValidAssetHandle(AssetHandle handle)
	{
		return Project::GetActive()->GetAssetManager()->IsValidAssetHandle(handle);
	}

	bool AssetManager::IsAssetLoaded(AssetHandle handle)
	{
		return Project::GetActive()->GetAssetManager()->IsAssetLoaded(handle);
	}

}
