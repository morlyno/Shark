#pragma once

#include "Shark/Serialization/SerializerBase.h"

#include <yaml-cpp/yaml.h>

namespace Shark {

	class Scene;
	class Entity;

	class SceneSerializer
	{
	public:
		SceneSerializer(Ref<Scene> scene);

		Ref<Scene> GetScene() const { return m_Scene; }
		const std::string& GetErrorMessage() const { return m_ErrorMessage; }

	public:
		void Serialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath);
		void SerializeToYAML(YAML::Emitter& out);
		bool DeserializeFromYAML(YAML::Node& sceneNode);

	public:
		void SerializeEntity(YAML::Emitter& out, Entity entity);
		void DeserializeEntity(YAML::Node& entityNode);

	private:
		Ref<Scene> m_Scene;
		std::string m_ErrorMessage;
	};

	class SceneAssetSerializer : public SerializerBase
	{
	public:
		virtual bool Serialize(Ref<Asset> asset, const AssetMetaData& metadata) override;
		virtual bool TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata) override;
	};

}
