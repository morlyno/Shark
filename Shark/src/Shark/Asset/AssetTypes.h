#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	enum class AssetType
	{
		None = 0,
		Scene = 1,
		Texture = 2,
		TextureSource = 3,
		ScriptFile = 4,
		Font
	};

	inline std::string ToString(AssetType assetType)
	{
		switch (assetType)
		{
			case AssetType::None: return "None";
			case AssetType::Scene: return "Scene";
			case AssetType::Texture: return "Texture";
			case AssetType::TextureSource: return "TextureSource";
			case AssetType::ScriptFile: return "ScriptFile";
			case AssetType::Font: return "Font";
		}

		SK_CORE_ASSERT(false, "Unkown AssetType");
		return "Unkown";
	}

	inline AssetType StringToAssetType(const std::string& assetType)
	{
		if (assetType == "None") return AssetType::None;
		if (assetType == "Scene") return AssetType::Scene;
		if (assetType == "Texture") return AssetType::Texture;
		if (assetType == "TextureSource") return AssetType::TextureSource;
		if (assetType == "ScriptFile") return AssetType::ScriptFile;
		if (assetType == "Font") return AssetType::Font;

		SK_CORE_ASSERT(false, "Unkown AssetType");
		return AssetType::None;
	}

	inline const std::unordered_map<std::string, AssetType> AssetExtensionMap = {
		{ ".skscene", AssetType::Scene },
		{ ".sktex", AssetType::Texture },
		{ ".png", AssetType::TextureSource },
		{ ".cs", AssetType::ScriptFile },
		{ ".ttf", AssetType::Font }
	};

}
