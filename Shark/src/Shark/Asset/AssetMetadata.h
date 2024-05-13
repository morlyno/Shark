#pragma once

#include "Asset.h"

namespace Shark {

	enum class AssetStatus
	{
		None = 0, Ready = 1, Loading = 2
	};

	struct AssetMetaData
	{
		AssetHandle Handle;
		AssetType Type = AssetType::None;
		std::filesystem::path FilePath; // relative to Assets (not Project!)
		bool IsMemoryAsset = false;
		bool IsEditorAsset = false;

		AssetStatus Status = AssetStatus::None;
		uint64_t LastWriteTime = 0; // Last write time by the time the asset got loaded

		bool IsValid() const { return Handle && !IsMemoryAsset; }
	};

	struct AssetLoadRequest
	{
		AssetMetaData Metadata;
		Ref<Asset> Asset;
		bool Reload = false;

		Threading::Future<Ref<Shark::Asset>> Future;

		AssetLoadRequest(const AssetMetaData& metadata, bool reload = false)
			: Metadata(metadata), Reload(reload), Future(true) {}
	};

	template<typename TAsset>
	struct AsyncLoadResult
	{
		Ref<TAsset> Asset;
		bool Ready;

		operator Ref<TAsset>() { return Asset; }
		operator bool() { return Ready; }

		AsyncLoadResult() = default;
		AsyncLoadResult(Ref<TAsset> asset, bool ready)
			: Asset(asset), Ready(ready) {}

		template<typename TAsset2>
		explicit AsyncLoadResult(const AsyncLoadResult<TAsset2>& other)
			: Asset(other.Asset.As<TAsset>()), Ready(other.Ready) {}

	};

}