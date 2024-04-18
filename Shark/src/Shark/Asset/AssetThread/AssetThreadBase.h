#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Asset/AssetMetadata.h"

namespace Shark {

	class AssetThreadBase : public RefCount
	{
	public:
		virtual ~AssetThreadBase() = default;

		virtual void QueueAssetLoad(const AssetLoadRequest& alr) = 0;
		virtual void RetrieveLoadedAssets(std::vector<AssetLoadRequest>& outLoadedAssets) = 0;
		virtual void UpdateLoadedAssetsMetadata(const std::unordered_map<AssetHandle, Ref<Asset>>& loadedAssets) = 0;
		virtual Threading::Future<Ref<Asset>> GetFuture(AssetHandle handle) = 0;
	};

}
