#pragma once

#include "Shark/Serialization/SerializerBase.h"

namespace Shark {

	class MeshSourceSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) override;
		
	};

}
