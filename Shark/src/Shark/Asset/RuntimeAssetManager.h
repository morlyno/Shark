#pragma once

#include "Shark/Asset/AssetManagerBase.h"

namespace Shark {

	class RuntimeAssetManager : public AssetManagerBase
	{
	public:
		RuntimeAssetManager() = default;
		~RuntimeAssetManager() = default;

		virtual Ref<Asset> GetAsset(AssetHandle handle) override { return nullptr; }
		virtual AssetHandle AddMemoryAsset(Ref<Asset> asset) override { return 0; }

		virtual AssetType GetAssetType(AssetHandle handle) const override { return AssetType::None; }
		virtual bool IsMemoryAsset(AssetHandle handle) const override { return false; }
		virtual bool IsValidAssetHandle(AssetHandle handle) const override { return false; }
		virtual bool IsAssetLoaded(AssetHandle handle) const override { return false; }

		virtual bool SaveAsset(AssetHandle handle) override { return false; }
		virtual bool ReloadAsset(AssetHandle handle) override { return false; }

		virtual void DeleteAsset(AssetHandle handle) override {}
		virtual void DeleteMemoryAsset(AssetHandle handle) override {}

		virtual bool EnsureCurrent(AssetHandle handle) override { return false; };
	};

}
