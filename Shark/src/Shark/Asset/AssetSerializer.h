#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"

namespace Shark {

	class AssetSerializer
	{
	public:
		static void RegisterSerializers();
		static void ReleaseSerializers();

		static bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata);
		static bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata);

		static Ref<Asset> TryLoad(AssetType assetType, const std::filesystem::path& assetPath);
		static bool Deserialize(Ref<Asset> asset, const std::filesystem::path& assetPath);

		template<typename TAsset>
		static Ref<TAsset> TryLoad(const std::filesystem::path& assetPath)
		{
			return TryLoad(TAsset::GetStaticType(), assetPath).As<TAsset>();
		}

	};

}

