#pragma once

#include "Shark/Serialization/SerializerBase.h"

namespace Shark {
	class StreamReader;
	class StreamWriter;

	class Mesh;
	class AnimationAsset;
	class AnimationGraphAsset;
}

namespace YAML {
	class Node;
	class Emitter;
}

namespace Shark {

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//// MeshSource Serializer /////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	class MeshSourceSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context) override;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//// Mesh Serializer ///////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	class MeshSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context) override;

	private:
		std::string SerializeToYAML(Ref<Mesh> mesh);
		bool DeserializeFromYAML(Ref<Mesh>& mesh, const std::string& filedata, AssetLoadContext* context);

	};

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//// Animation Serializer //////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	class AnimationSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context) override;

	private:
		bool SerializeToYAML(Ref<AnimationAsset> animation, StreamWriter* stream);
		bool DeserializeFromYAML(Ref<AnimationAsset>& animation, StreamReader* stream, AssetLoadContext* context);

	};

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//// AnimationGraph Serializer (Runtime) ///////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	class AnimationGraphSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context) override;

	private:
		bool SerializeToYAML(Ref<AnimationGraphAsset> animationGraph, StreamWriter* stream);
		bool DeserializeFromYAML(Ref<AnimationGraphAsset>& animationGraph, StreamReader* stream, AssetLoadContext* context);

		static bool SerializeGraphToYAML(Ref<AnimationGraphAsset> animationGraph, YAML::Emitter& emitter);
		static bool DeserializeGraphFromYAML(Ref<AnimationGraphAsset> animationGraph, const YAML::Node& animationGraphNode, AssetLoadContext* context);

		friend class EditorAnimationGraphSerializer;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////
	//// AnimationGraph Serializer (Editor) ////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////

	// 
	// Defined in Shark-Editor AnimationGraph/EditorAnimationGraphSerializer
	// 

}
