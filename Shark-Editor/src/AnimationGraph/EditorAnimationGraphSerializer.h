#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Serialization/SerializerBase.h"

namespace Shark {
	class StreamReader;
	class StreamWriter;

	class EditorAnimationGraphAsset;
}

namespace Shark {

	class EditorAnimationGraphSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context) override;

	private:
		bool SerializeToYAML(Ref<EditorAnimationGraphAsset> animationGraph, StreamWriter* stream);
		bool DeserializeFromYAML(Ref<EditorAnimationGraphAsset>& animationGraph, StreamReader* stream, AssetLoadContext* context);

	};

}
