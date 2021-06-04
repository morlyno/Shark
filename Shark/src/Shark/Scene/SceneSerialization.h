#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"

namespace Shark {

	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& scene);
		~SceneSerializer() = default;

		bool Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);
	private:
		Ref<Scene> m_Scene;
	};

}
