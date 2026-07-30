#pragma once

#include "Shark/Serialization/SerializerBase.h"

namespace Shark {
	class StreamReader;
	class StreamWriter;

	class Mesh;
	class AnimationAsset;
}

namespace Shark {

	class MeshSourceSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context) override;
	};

	class MeshSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context) override;

	private:
		std::string SerializeToYAML(Ref<Mesh> mesh);
		bool DeserializeFromYAML(Ref<Mesh>& mesh, const std::string& filedata, AssetLoadContext* context);

	};

	class AnimationSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context) override;

	private:
		bool SerializeToYAML(Ref<AnimationAsset> animation, StreamWriter* stream);
		bool DeserializeFromYAML(Ref<AnimationAsset>& animation, StreamReader* stream, AssetLoadContext* context);

	};

}
