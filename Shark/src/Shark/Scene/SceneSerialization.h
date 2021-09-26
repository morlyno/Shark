#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"

#include <yaml-cpp/yaml.h>

namespace Shark {

	class SceneSerializer
	{
	public:
		static constexpr uint64_t SceneSerializerVersion = 2;
	public:
		SceneSerializer(const Ref<Scene>& scene);
		~SceneSerializer() = default;

		bool Serialize() { return Serialize(m_Scene->GetFilePath()); }
		bool Deserialize() { return Deserialize(m_Scene->GetFilePath()); }
		bool Serialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath);
	private:
		bool LagacyModeDeserialize(YAML::Node& in, uint64_t version);
	private:
		Ref<Scene> m_Scene;
	};

}
