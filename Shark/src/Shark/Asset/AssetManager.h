
#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Core/Project.h"

namespace Shark {

	class AssetManager
	{
	public:
		static Ref<Asset> GetAsset(AssetHandle handle) { return Project::GetActiveEditorAssetManager()->GetAsset(handle); }
		static AssetHandle AddMemoryAsset(Ref<Asset> asset) { return Project::GetActive()->GetAssetManager()->AddMemoryAsset(asset); }

		static AssetType GetAssetType(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->GetAssetType(handle); }
		static bool IsMemoryAsset(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->IsMemoryAsset(handle); }
		static bool IsValidAssetHandle(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->IsValidAssetHandle(handle); }
		static bool IsAssetLoaded(AssetHandle handle) { return Project::GetActive()->GetAssetManager()->IsAssetLoaded(handle); }

		static bool SaveAsset(AssetHandle handle) { return Project::GetActiveAssetManager()->SaveAsset(handle); }
		static bool ReloadAsset(AssetHandle handle) { return Project::GetActiveAssetManager()->ReloadAsset(handle); }

		static void DeleteAsset(AssetHandle handle) { return Project::GetActiveAssetManager()->DeleteAsset(handle); }
		static void DeleteMemoryAsset(AssetHandle handle) { return Project::GetActiveAssetManager()->DeleteMemoryAsset(handle); }
		
		static bool EnsureCurrent(AssetHandle handle) { return Project::GetActiveAssetManager()->EnsureCurrent(handle); }

		template<typename TAsset>
		static Ref<TAsset> GetAsset(AssetHandle handle)
		{
			static_assert(std::is_base_of_v<Asset, TAsset>, "GetAsset only works for types with base class Asset");

			Ref<Asset> asset = GetAsset(handle);
			if (asset && asset->GetAssetType() != TAsset::GetStaticType())
				return nullptr;

			return asset.As<TAsset>();
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
