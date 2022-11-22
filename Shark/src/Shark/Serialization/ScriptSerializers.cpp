#include "skpch.h"
#include "ScriptSerializers.h"

#include "Shark/Asset/Assets.h"

namespace Shark {

	bool ScriptFileSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_CORE_VERIFY(asset);
		SK_CORE_INFO_TAG("Serialization", "Serializing ScriptFile to {}", metadata.FilePath);
		Timer timer;

		SK_CORE_INFO_TAG("Serialization", "Serializing ScriptFile took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

	bool ScriptFileSerializer::Deserialize(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_CORE_INFO_TAG("Serialization", "Deserializing ScriptFile from {}", metadata.FilePath);
		Timer timer;

		asset = Ref<ScriptFile>::Create();
		asset->Handle = metadata.Handle;

		SK_CORE_INFO_TAG("Serialization", "Deserializing ScriptFile took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

}
