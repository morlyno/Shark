#pragma once

#include "Shark/Serialization/SerializerBase.h"

namespace Shark {

	class Texture2D;
	class TextureSource;
	class Image2D;

	class TextureSourceSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) override;
		virtual bool Deserialize(Ref<Asset> asset, const std::filesystem::path& assetPath) override;

		bool TryLoadAssetFromTexture(Ref<TextureSource>& textureSource, const std::filesystem::path& filepath);
	};

	class TextureSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) override;
		virtual bool Deserialize(Ref<Asset> asset, const std::filesystem::path& assetPath) override;

	public:
		std::string SerializeToYAML(Ref<Texture2D> texture);
		bool DesrializeFromYAML(Ref<Texture2D> texture, const std::string& filedata);
	};

#if 0
	class ImageSerializer
	{
	public:
		ImageSerializer(Ref<Image2D> image);
		~ImageSerializer() = default;

		bool Serialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath);
	private:
		Ref<Image2D> m_Image;
	};
#endif

}
