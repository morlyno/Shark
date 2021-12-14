#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"

namespace Shark {

	class SerializerBase : public RefCount
	{
	public:
		virtual bool TryLoadData(Ref<Asset>& asset, const std::filesystem::path& filePath) = 0;

		virtual bool Serialize(Ref<Asset> asset, const std::filesystem::path& filePath) = 0;
		virtual bool Deserialize(Ref<Asset> asset, const std::filesystem::path& filePath) = 0;
	};

	class AssetSerializer
	{
	public:
		static bool TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata);

		static bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata);
		static bool Deserialize(Ref<Asset> asset, const AssetMetaData& metadata);
	};

}

