
#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Core/Threading.h"

namespace Shark {

	class AssetManager
	{
	public:
		static AssetType GetAssetType(AssetHandle handle);
		static Ref<Asset> GetAsset(AssetHandle handle);
		static Ref<Asset> GetAssetAsync(AssetHandle handle);
		static Threading::Future<Ref<Asset>> GetAssetFuture(AssetHandle handle);

		static bool WaitForAsset(AssetHandle handle, bool queueLoad = true);
		static void LoadAssetAsync(AssetHandle handle);

		static std::vector<AssetHandle> GetAllAssetsOfType(AssetType assetType);

		static AssetHandle AddMemoryAsset(Ref<Asset> asset);
		static bool ReloadAsset(AssetHandle handle);
		static void ReloadAssetAsync(AssetHandle handle);
		static bool DependenciesLoaded(AssetHandle handle, bool loadIfNotReady = false);
		static bool IsValidAssetHandle(AssetHandle handle);
		static bool IsMemoryAsset(AssetHandle handle);
		static bool IsAssetLoaded(AssetHandle handle);
		static void DeleteAsset(AssetHandle handle);
		static void DeleteMemoryAsset(AssetHandle handle);
		
		static void SyncWithAssetThread();

		template<typename TAsset>
		static Ref<TAsset> GetAsset(AssetHandle handle)
		{
			static_assert(std::is_base_of_v<Asset, TAsset>, "GetAsset only works for types with base class Asset");

			Ref<Asset> asset = GetAsset(handle);
			if (asset && asset->GetAssetType() != TAsset::GetStaticType())
			{
				SK_CORE_ERROR_TAG("AssetManager", "GetAsset<{}> Error asset {} is of type {}", TAsset::GetStaticType(), handle, asset->GetAssetType());
				return nullptr;
			}

			return asset.As<TAsset>();
		}
		
		template<typename TAsset>
		static Ref<TAsset> GetAssetAsync(AssetHandle handle)
		{
			static_assert(std::is_base_of_v<Asset, TAsset>, "GetAsset only works for types with base class Asset");

			auto asset = GetAssetAsync(handle);
			if (asset && asset->GetAssetType() != TAsset::GetStaticType())
			{
				SK_CORE_ERROR_TAG("AssetManager", "GetAssetAsync<{}> Error asset {} is of type {}", TAsset::GetStaticType(), handle, asset->GetAssetType());
				return nullptr;
			}

			return GetAssetAsync(handle).As<TAsset>();
		}

		template<typename TAsset>
		static Ref<TAsset> GetAssetAsync(AssetHandle handle, Ref<TAsset> defaultAsset)
		{
			static_assert(std::is_base_of_v<Asset, TAsset>, "GetAsset only works for types with base class Asset");
			if (auto asset = GetAssetAsync<TAsset>(handle))
				return asset;
			return defaultAsset;
		}

		template<typename TAsset>
		static Ref<TAsset> GetAssetAsync(AssetHandle handle, AssetHandle defaultAsset)
		{
			static_assert(std::is_base_of_v<Asset, TAsset>, "GetAsset only works for types with base class Asset");
			if (auto asset = GetAssetAsync<TAsset>(handle))
				return asset;
			return GetAssetAsync<TAsset>(defaultAsset);
		}

		template<typename TAsset, typename... TArgs>
		static AssetHandle CreateMemoryOnlyAsset(TArgs&&... args)
		{
			Ref<TAsset> asset = Ref<TAsset>::Create(std::forward<TArgs>(args)...);
			return AddMemoryAsset(asset);
		}

		template<typename TAsset, typename... TArgs>
		static AssetHandle CreateMemoryOnlyRendererAsset(TArgs&&... args)
		{
			Ref<TAsset> asset = TAsset::Create(std::forward<TArgs>(args)...);
			return AddMemoryAsset(asset);
		}

	};

}
