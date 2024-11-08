#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Core/Threading.h"

namespace Shark {

	enum class AssetStatus
	{
		Unloaded = 0, Ready = 1, Loading = 2
	};

	struct AssetMetaData
	{
		AssetHandle Handle;
		AssetType Type = AssetType::None;
		std::filesystem::path FilePath; // relative to Assets (not Project!)
		bool IsMemoryAsset = false;
		bool IsEditorAsset = false;

		AssetStatus Status = AssetStatus::Unloaded;
		uint64_t LastWriteTime = 0; // Last write time by the time the asset got loaded

		bool IsValid() const { return Handle && !IsMemoryAsset; }
	};

	struct AssetLoadRequest
	{
		AssetMetaData Metadata;
		Ref<Asset> Asset;
		bool Reload = false;

		Threading::Future<Ref<Shark::Asset>> Future;

		AssetLoadRequest() = default;
		AssetLoadRequest(const AssetMetaData& metadata, bool reload = false)
			: Metadata(metadata), Reload(reload), Future(true) {}
		AssetLoadRequest(AssetMetaData&& metadata, bool reload = false)
			: Metadata(std::move(metadata)), Reload(reload), Future(true) {}
	};

	template<typename TAsset>
	struct AsyncLoadResult
	{
		Ref<TAsset> Asset = nullptr;
		bool Ready = false;

		operator Ref<TAsset>() const { return Asset; }
		operator bool() const { return Ready; }

		auto operator->() { return Asset.operator ->(); }
		auto operator->() const { return Asset.operator ->(); }

		AsyncLoadResult() = default;
		AsyncLoadResult(Ref<TAsset> asset, bool ready)
			: Asset(asset), Ready(ready) {}

		template<typename TAsset2>
		explicit AsyncLoadResult(const AsyncLoadResult<TAsset2>& other)
			: Asset(other.Asset.As<TAsset>()), Ready(other.Ready) {}

	};

}