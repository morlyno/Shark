#pragma once
#include "SerializerBase.h"

namespace Shark {

	class ScriptFileSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool Deserialize(Ref<Asset>& asset, const AssetMetaData& metadata) override;
	};

}
