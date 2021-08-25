#pragma once

#include "Shark/Scene/Scene.h"

namespace Shark {

	namespace SceneManagerUtils {

		uint32_t GetProjectIndexFromName(const std::string& sceneName);
		std::string GetSceneNameFromProjectIndex(uint32_t index);

	}

	class SceneManager
	{
	public:
		static Ref<Scene> LoadSceneByName(const std::string& sceneName);
		static Ref<Scene> LoadSceneByIndex(uint32_t index);

		static void SetActiveScene(const Ref<Scene>& scene);
		static Ref<Scene> GetActiveScene();
	};

}
