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
		virtual bool Deserialize(Ref<Asset>& asset, const AssetMetaData& metadata) override;

		bool DeserializeFromTexture(Ref<TextureSource>& textureSource, const std::filesystem::path& filepath);
	};

	class TextureSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool Deserialize(Ref<Asset>& asset, const AssetMetaData& metadata) override;

	public:
		std::string SerializeToYAML(Ref<Texture2D> texture);
		bool DesrializeFromYAML(Ref<Texture2D> texture, const std::string& filedata);

		// bool SerializeToAssetPack(Stream& stream, Ref<Texture2D> texture);
		// bool DeserializeFromAssetPack(Stream& stream, Ref<Texture2D> texture);
	};


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

}
