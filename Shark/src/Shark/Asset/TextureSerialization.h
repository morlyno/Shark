#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Asset/AssetSerializer.h"
#include "Shark/Render/Texture.h"

namespace Shark {

	class TextureSerializer : public SerializerBase
	{
	public:
		virtual bool TryLoadData(Ref<Asset>& asset, const std::filesystem::path& filePath) override;

		virtual bool Serialize(Ref<Asset> asset, const std::filesystem::path& filePath) override;
		virtual bool Deserialize(Ref<Asset> asset, const std::filesystem::path& filePath) override;
	};

#if SK_TEXTURE_SOURCE
	class TextureSourceSerializer : public SerializerBase
	{
	public:
		virtual bool TryLoadData(Ref<Asset>& asset, const std::filesystem::path& filePath) override;

		virtual bool Serialize(Ref<Asset> asset, const std::filesystem::path& filePath) override;
		virtual bool Deserialize(Ref<Asset> asset, const std::filesystem::path& filePath) override;
	};
#endif

}
