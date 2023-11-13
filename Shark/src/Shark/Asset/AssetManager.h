#pragma once

#include "Shark/Asset/Asset.h"

namespace Shark {

	class AssetManager
	{
	public:
		static Ref<Asset> GetAsset(AssetHandle handle);
		static AssetHandle AddMemoryAsset(Ref<Asset> asset);

		static AssetType GetAssetType(AssetHandle handle);
		static bool IsMemoryAsset(AssetHandle handle);
		static bool IsValidAssetHandle(AssetHandle handle);
		static bool IsAssetLoaded(AssetHandle handle);

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
		static AssetHandle CreateMemoryAsset(TArgs&&... args)
		{
			Ref<TAsset> asset = TAsset::Create(std::forward<TArgs>(args)...);
			return AddMemoryAsset(asset);
		}
	};

}
