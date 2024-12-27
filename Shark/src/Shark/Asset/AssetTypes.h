#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	enum class AssetType
	{
		None = 0,
		Scene,
		Texture,
		ScriptFile,
		Font,
		MeshSource,
		Mesh,
		Material,
		Environment,
		Prefab
	};

	inline const std::unordered_map<std::string, AssetType> AssetExtensionMap = {
		{ ".skscene", AssetType::Scene },
		{ ".sktex", AssetType::Texture },
		{ ".png", AssetType::Texture },
		{ ".jpg", AssetType::Texture },
		{ ".jpeg", AssetType::Texture },
		{ ".cs", AssetType::ScriptFile },
		{ ".ttf", AssetType::Font },
		{ ".obj", AssetType::MeshSource },
		{ ".fbx", AssetType::MeshSource },
		{ ".gltf", AssetType::MeshSource },
		{ ".glb", AssetType::MeshSource },
		{ ".skmesh", AssetType::Mesh },
		{ ".skmat", AssetType::Material },
		{ ".hdr", AssetType::Environment },
		{ ".sfab", AssetType::Prefab }
	};

}
