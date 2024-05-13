
#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Core/Project.h"

namespace Shark {

	class AssetManager
	{
	public:
		static AssetType GetAssetType(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->GetAssetType(handle); }
		static Ref<Asset> GetAsset(AssetHandle handle) { return Project::GetActiveEditorAssetManager()->GetAsset(handle); }
		static AsyncLoadResult<Asset> GetAssetAsync(AssetHandle handle) { return Project::GetActiveEditorAssetManager()->GetAssetAsync(handle); }
		static Threading::Future<Ref<Asset>> GetAssetFuture(AssetHandle handle) { return Project::GetActiveEditorAssetManager()->GetAssetFuture(handle); }

		static AssetHandle AddMemoryAsset(Ref<Asset> asset) { return Project::GetActive()->GetAssetManager()->AddMemoryAsset(asset); }
		static bool ReloadAsset(AssetHandle handle) { return Project::GetActiveAssetManager()->ReloadAsset(handle); }
		static void ReloadAssetAsync(AssetHandle handle) { Project::GetActiveAssetManager()->ReloadAssetAsync(handle); }
		static bool IsFullyLoaded(AssetHandle handle, bool loadIfNotReady = false) { return Project::GetActiveAssetManager()->IsFullyLoaded(handle, loadIfNotReady); }
		static bool IsValidAssetHandle(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->IsValidAssetHandle(handle); }
		static bool IsMemoryAsset(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->IsMemoryAsset(handle); }
		static bool IsAssetLoaded(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->IsAssetLoaded(handle); }
		static void DeleteAsset(AssetHandle handle) { return Project::GetActiveAssetManager()->DeleteAsset(handle); }
		static void DeleteMemoryAsset(AssetHandle handle) { return Project::GetActiveAssetManager()->DeleteMemoryAsset(handle); }
		
		static void SyncWithAssetThread() { Project::GetActiveAssetManager()->SyncWithAssetThread(); }

		template<typename TAsset>
		static Ref<TAsset> GetAsset(AssetHandle handle)
		{
			static_assert(std::is_base_of_v<Asset, TAsset>, "GetAsset only works for types with base class Asset");

			Ref<Asset> asset = GetAsset(handle);
			if (asset && asset->GetAssetType() != TAsset::GetStaticType())
				return nullptr;

			return asset.As<TAsset>();
		}
		
		template<typename TAsset>
		static AsyncLoadResult<TAsset> GetAssetAsync(AssetHandle handle)
		{
			static_assert(std::is_base_of_v<Asset, TAsset>, "GetAsset only works for types with base class Asset");

			AsyncLoadResult<Asset> result = GetAssetAsync(handle);
			return AsyncLoadResult<TAsset>(result);
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
