#pragma once

#include "Shark/Asset/AssetManager/AssetManagerBase.h"

namespace Shark {

	class RuntimeAssetManager : public AssetManagerBase
	{
	public:
		RuntimeAssetManager() = default;
		~RuntimeAssetManager() = default;

		virtual AssetType GetAssetType(AssetHandle handle) { return AssetType::None; }
		virtual Ref<Asset> GetAsset(AssetHandle handle) { return nullptr; }
		virtual AsyncLoadResult<Asset> GetAssetAsync(AssetHandle handle) { return {}; }
		virtual Threading::Future<Ref<Asset>> GetAssetFuture(AssetHandle handle) { return {}; }

		virtual AssetHandle AddMemoryAsset(Ref<Asset> asset) { return AssetHandle::Invalid; }
		virtual bool ReloadAsset(AssetHandle handle) { return false; }
		virtual void ReloadAssetAsync(AssetHandle handle) {}
		virtual bool IsFullyLoaded(AssetHandle handle, bool loadIfNotReady) { return false; }
		virtual bool IsValidAssetHandle(AssetHandle handle) { return false; }
		virtual bool IsMemoryAsset(AssetHandle handle) { return false; }
		virtual bool IsAssetLoaded(AssetHandle handle) { return false; }
		virtual void DeleteAsset(AssetHandle handle) {}
		virtual void DeleteMemoryAsset(AssetHandle handle) {}

		virtual void WaitUntilIdle() {}
		virtual void SyncWithAssetThread() {}
	};

}
