#pragma once

#include "Shark/Asset/Asset.h"

namespace Shark {

	using AssetsMap = std::unordered_map<AssetHandle, Ref<Asset>>;

	class AssetManagerBase : public RefCount
	{
	public:
		virtual ~AssetManagerBase() = default;

		virtual Ref<Asset> GetAsset(AssetHandle handle) = 0;
		virtual AssetHandle AddMemoryAsset(Ref<Asset> asset) = 0;

		virtual AssetType GetAssetType(AssetHandle handle) const = 0;
		virtual bool IsMemoryAsset(AssetHandle handle) const = 0;
		virtual bool IsValidAssetHandle(AssetHandle handle) const = 0;
		virtual bool IsAssetLoaded(AssetHandle handle) const = 0;

	};

}