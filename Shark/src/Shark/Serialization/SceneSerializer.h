#pragma once

#include "Shark/Serialization/SerializerBase.h"

namespace Shark {

	class Scene;

	class SceneSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool Deserialize(Ref<Asset>& asset, const AssetMetaData& metadata) override;

	private:
		std::string SerializeToYAML(Ref<Scene> scene);
		bool DeserializeFromYAML(Ref<Scene> scene, const std::string& filedata);
	};

}
