#pragma once

#include "Shark/Serialization/SerializerBase.h"

namespace Shark {

	class Texture2D;
	class TextureSource;

	class TextureSourceSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) override;

		bool TryLoadAssetFromTexture(Ref<TextureSource>& textureSource, const std::filesystem::path& filepath);
	};

	class TextureSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) override;

	public:
		std::string SerializeToYAML(Ref<Texture2D> texture);
		bool DesrializeFromYAML(Ref<Texture2D> texture, const std::string& filedata);
	};

}
