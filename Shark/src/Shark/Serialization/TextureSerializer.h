#pragma once

#include "Shark/Serialization/SerializerBase.h"

namespace Shark {

	class Texture2D;
	struct TextureSpecification;

	class TextureSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context) override;

	private:
		std::string SerializeToYAML(Ref<Texture2D> texture);
		bool DeserializeFromYAML(const std::string& filedata, TextureSpecification& outSpecification, AssetHandle& outSourceHandle, AssetLoadContext* context);
	};

}
