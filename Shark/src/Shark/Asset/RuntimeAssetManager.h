#pragma once

#include "Shark/Asset/AssetManagerBase.h"

namespace Shark {

	class RuntimeAssetManager : public AssetManagerBase
	{
	public:
		RuntimeAssetManager();
		~RuntimeAssetManager();

		virtual Ref<Asset> GetAsset(AssetHandle handle) override;
		virtual AssetHandle AddMemoryAsset(Ref<Asset> asset) override;

		virtual AssetType GetAssetType(AssetHandle handle) const override;
		virtual bool IsMemoryAsset(AssetHandle handle) const override;
		virtual bool IsValidAssetHandle(AssetHandle handle) const override;
		virtual bool IsAssetLoaded(AssetHandle handle) const override;

	private:
		AssetsMap m_LoadedAssets;
		std::set<AssetHandle> m_MemoryAssets;
	};

}
