#pragma once

#include "Shark/Core/Thread.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Asset/AssetMetadata.h"

namespace Shark {

	using LoadedAssetsMap = std::unordered_map<AssetHandle, Ref<Asset>>;

	class AssetManagerBase : public RefCount
	{
	public:
		AssetManagerBase() = default;
		virtual ~AssetManagerBase() = default;

		virtual AssetType GetAssetType(AssetHandle handle) = 0;
		virtual Ref<Asset> GetAsset(AssetHandle handle) = 0;
		virtual AsyncLoadResult<Asset> GetAssetAsync(AssetHandle handle) = 0;
		virtual Threading::Future<Ref<Asset>> GetAssetFuture(AssetHandle handle) = 0;

		virtual AssetHandle AddMemoryAsset(Ref<Asset> asset) = 0;
		virtual bool ReloadAsset(AssetHandle handle) = 0;
		virtual void ReloadAssetAsync(AssetHandle handle) = 0;
		virtual bool IsFullyLoaded(AssetHandle handle, bool loadIfNotReady) = 0;
		virtual bool IsValidAssetHandle(AssetHandle handle) = 0;
		virtual bool IsMemoryAsset(AssetHandle handle) = 0;
		virtual bool IsAssetLoaded(AssetHandle handle) = 0;
		virtual void DeleteAsset(AssetHandle handle) = 0;
		virtual void DeleteMemoryAsset(AssetHandle handle) = 0;

		virtual void WaitUntilIdle() = 0;
		virtual void SyncWithAssetThread() = 0;
	};

}
