#pragma once

#include "Shark/Asset/Asset.h"

namespace Shark {

	class Serializer
	{
	public:
		virtual bool TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata) = 0;
		virtual bool Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata) = 0;

	};

	class SceneSerializer : public Serializer
	{
	public:
		virtual bool TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata) override;
		virtual bool Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata) override;
	};

	class TextureSerializer : public Serializer
	{
	public:
		virtual bool TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata) override;
		virtual bool Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata) override;

		std::filesystem::path DeserializeSourcePath(const std::filesystem::path& filePath);
	};

	class TextureSourceSerializer : public Serializer
	{
	public:
		virtual bool TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata) override;
		virtual bool Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata) override;

	};

	class ScriptFileSerializer : public Serializer
	{
	public:
		virtual bool TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata) override;
		virtual bool Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata) override;

	};

}
