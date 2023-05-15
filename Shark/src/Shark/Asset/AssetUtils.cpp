#include "skpch.h"
#include "AssetUtils.h"

#include "Shark/Asset/Asset.h"
#include "Shark/Asset/AssetTypes.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Render/Texture.h"
#include "Shark/Asset/Assets.h"
#include "Shark/Render/Font.h"

namespace Shark {

	Ref<Asset> AssetUtils::Create(AssetType assetType)
	{
		switch (assetType)
		{
			case AssetType::Scene: return Ref<Scene>::Create();
			case AssetType::Texture: return Texture2D::Create();
			case AssetType::TextureSource: return TextureSource::Create();
			case AssetType::ScriptFile: return ScriptFile::Create();
			case AssetType::Font: return Font::Create();
		}

		SK_CORE_VERIFY(false, "Unkown AssetType");
		return nullptr;
	}

	AssetType AssetUtils::GetAssetTypeFromPath(const std::filesystem::path& assetPath)
	{
		std::string extension = assetPath.extension().string();
		if (AssetExtensionMap.find(extension) != AssetExtensionMap.end())
			return AssetExtensionMap.at(extension);
		return AssetType::None;
	}

}
