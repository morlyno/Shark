#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"

namespace Shark {

	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& scene);
		~SceneSerializer() = default;

		bool Serialize() { return Serialize(m_Scene->GetFilePath()); }
		bool Deserialize() { return Deserialize(m_Scene->GetFilePath()); }
		bool Serialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath);
	private:
		Ref<Scene> m_Scene;
	};

}
