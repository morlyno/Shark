#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Core/Project.h"

namespace Shark {
	
	class AssetThread
	{
	public:
		static void QueueAssetLoad(const AssetLoadRequest& alr) { Project::GetActive()->GetAssetThread()->QueueAssetLoad(alr); }
		static void RetrieveLoadedAssets(std::vector<AssetLoadRequest>& outLoadedAssets) { Project::GetActive()->GetAssetThread()->RetrieveLoadedAssets(outLoadedAssets); }
		static void UpdateLoadedAssetsMetadata(const std::unordered_map<AssetHandle, Ref<Asset>>& loadedAssets) { Project::GetActive()->GetAssetThread()->UpdateLoadedAssetsMetadata(loadedAssets); }
		static Threading::Future<Ref<Asset>> GetFuture(AssetHandle handle) { return Project::GetActive()->GetAssetThread()->GetFuture(handle); }
	};

}
