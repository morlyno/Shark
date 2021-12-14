#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	enum class AssetType
	{
		None = 0,
		Scene = 1,
		Texture = 2
	};

	inline std::string AssetTypeToString(AssetType assetType)
	{
		switch (assetType)
		{
			case AssetType::None: return "None";
			case AssetType::Scene: return "Scene";
			case AssetType::Texture: return "Texture";
		}

		SK_CORE_ASSERT(false, "Unkown AssetType");
		return "Unkown";
	}

	inline AssetType StringToAssetType(const std::string& assetType)
	{
		if (assetType == "None") return AssetType::None;
		if (assetType == "Scene") return AssetType::Scene;
		if (assetType == "Texture") return AssetType::Texture;

		SK_CORE_ASSERT(false, "Unkown AssetType");
		return AssetType::None;
	}

	inline std::unordered_map<std::string, AssetType> AssetExtentionMap = {
		{ ".skscene", AssetType::Scene },
		{ ".png", AssetType::Texture}
	};

	inline bool IsValidExtention(const std::string& extention)
	{
		return AssetExtentionMap.find(extention) != AssetExtentionMap.end();
	}

}
