#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Asset/AssetSerializer.h"
#include "Shark/Render/Texture.h"

#include <filesystem>

namespace Shark {

	class TextureSerializer : public Serializer
	{
	public:
		virtual bool TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata) override;
		virtual bool Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata) override;

		bool Deserialize(Ref<Texture2D>& textureAsset, const std::filesystem::path& filePath);
		std::filesystem::path DeserializeSourcePath(const std::filesystem::path& filePath);
	};

}
