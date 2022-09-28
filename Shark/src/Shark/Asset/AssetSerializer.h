#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"

namespace Shark {

	class AssetSerializer
	{
	public:
		static void RegisterSerializers();
		static void ReleaseSerializers();

		static bool TryLoadData(Ref<Asset>& asset, const AssetMetaData& metadata);
		static bool Serialize(const Ref<Asset>& asset, const AssetMetaData& metadata);
	};

}

