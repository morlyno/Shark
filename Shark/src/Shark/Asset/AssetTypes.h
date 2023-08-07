#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	enum class AssetType
	{
		None = 0,
		Scene,
		Texture,
		TextureSource,
		ScriptFile,
		Font,
		MeshSource,
		Mesh,
		Material
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
			case AssetType::MeshSource: return "MeshSource";
			case AssetType::Mesh: return "Mesh";
			case AssetType::Material: return "Material";
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
		if (assetType == "MeshSource") return AssetType::MeshSource;
		if (assetType == "Mesh") return AssetType::Mesh;
		if (assetType == "Material") return AssetType::Material;

		SK_CORE_ASSERT(false, "Unkown AssetType");
		return AssetType::None;
	}

	inline const std::unordered_map<std::string, AssetType> AssetExtensionMap = {
		{ ".skscene", AssetType::Scene },
		{ ".sktex", AssetType::Texture },
		{ ".png", AssetType::TextureSource },
		{ ".cs", AssetType::ScriptFile },
		{ ".ttf", AssetType::Font },
		{ ".obj", AssetType::MeshSource },
		{ ".skmesh", AssetType::Mesh },
		{ ".skmat", AssetType::Material }
	};

}
