
#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Core/Project.h"

namespace Shark {

	class AssetManager
	{
	public:
		static AssetType GetAssetType(AssetHandle handle) { return Project::GetAssetManager()->GetAssetType(handle); }
		static Ref<Asset> GetAsset(AssetHandle handle) { return Project::GetEditorAssetManager()->GetAsset(handle); }
		static Ref<Asset> GetAssetAsync(AssetHandle handle) { return Project::GetEditorAssetManager()->GetAssetAsync(handle); }
		static Threading::Future<Ref<Asset>> GetAssetFuture(AssetHandle handle) { return Project::GetEditorAssetManager()->GetAssetFuture(handle); }

		static bool WaitForAsset(AssetHandle handle, bool queueLoad = true) { return Project::GetAssetManager()->WaitForAsset(handle, queueLoad); }
		static void LoadAssetAsync(AssetHandle handle) { return Project::GetAssetManager()->LoadAssetAsync(handle); }

		static std::vector<AssetHandle> GetAllAssetsOfType(AssetType assetType) { return Project::GetAssetManager()->GetAllAssetsOfType(assetType); }

		static AssetHandle AddMemoryAsset(Ref<Asset> asset) { return Project::GetAssetManager()->AddMemoryAsset(asset); }
		static bool ReloadAsset(AssetHandle handle) { return Project::GetAssetManager()->ReloadAsset(handle); }
		static void ReloadAssetAsync(AssetHandle handle) { Project::GetAssetManager()->ReloadAssetAsync(handle); }
		static bool DependenciesLoaded(AssetHandle handle, bool loadIfNotReady = false) { return Project::GetAssetManager()->DependenciesLoaded(handle, loadIfNotReady); }
		static bool IsValidAssetHandle(AssetHandle handle) { return Project::GetAssetManager()->IsValidAssetHandle(handle); }
		static bool IsMemoryAsset(AssetHandle handle) { return Project::GetAssetManager()->IsMemoryAsset(handle); }
		static bool IsAssetLoaded(AssetHandle handle) { return Project::GetAssetManager()->IsAssetLoaded(handle); }
		static void DeleteAsset(AssetHandle handle) { return Project::GetAssetManager()->DeleteAsset(handle); }
		static void DeleteMemoryAsset(AssetHandle handle) { return Project::GetAssetManager()->DeleteMemoryAsset(handle); }
		
		static void SyncWithAssetThread() { Project::GetAssetManager()->SyncWithAssetThread(); }

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
