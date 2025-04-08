#pragma once

#include "Shark/Serialization/SerializerBase.h"

namespace Shark {

	class Texture2D;
	struct TextureSpecification;

	class TextureSerializer : public SerializerBase
	{
	public:
		static AssetType GetAssetType() { return AssetType::Texture; }
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) override;

		Ref<Texture2D> TryLoad(const std::filesystem::path& filepath, bool useFallback = false);
		bool LoadImageData(const std::filesystem::path& filepath, TextureSpecification& outSpecification, Buffer& outBuffer);

	private:
		std::string SerializeToYAML(Ref<Texture2D> texture);
		bool DesrializeFromYAML(const std::string& filedata, TextureSpecification& outSpecification, AssetHandle& outSourceHandle, Buffer& outImageData);
		bool LoadImageDataFromSource(const std::filesystem::path& filepath, TextureSpecification& outSpecification, Buffer& outBuffer);
	};

}
