#include "skpch.h"
#include "SceneManager.h"
#include "Shark/Core/Project.h"
#include "Shark/Core/Application.h"
#include "Shark/Utility/FileSystem.h"
#include "Shark/Scene/SceneSerialization.h"

namespace Shark {

	namespace SceneManagerUtils {

		uint32_t GetProjectIndexFromName(const std::string& sceneName)
		{
			const auto& proj = Application::Get().GetProject();
			for (uint32_t i = 0; i < proj.GetNumScenes(); i++)
			{
				auto& scene = proj.GetSceneAt(i);
				if (FileSystem::FileName(scene) == sceneName)
					return i;
			}
			SK_CORE_ASSERT(false, "Scene not found");
			return (uint32_t)-1;
		}

		std::string GetSceneNameFromProjectIndex(uint32_t index)
		{
			const auto& proj = Application::Get().GetProject();
			SK_CORE_ASSERT(index < proj.GetNumScenes(), "Index out of range");
			return std::move(FileSystem::FileName(proj.GetSceneAt(index)));
		}

	}

	static Ref<Scene> s_ActiveScene;

	Ref<Scene> SceneManager::LoadSceneByName(const std::string& sceneName)
	{
		return LoadSceneByIndex(SceneManagerUtils::GetProjectIndexFromName(sceneName));
	}

	Ref<Scene> SceneManager::LoadSceneByIndex(uint32_t index)
	{
		const auto& proj = Application::Get().GetProject();
		auto scene = Ref<Scene>::Create();
		scene->SetFilePath(proj.GetSceneAt(index));
		SceneSerializer serializer(scene);
		if (!serializer.Deserialize())
		{
			SK_CORE_ASSERT(false, "Failed to deserialize Scene");
			return nullptr;
		}
		SetActiveScene(scene);
		return scene;
	}

	void SceneManager::SetActiveScene(const Ref<Scene>& scene)
	{
		s_ActiveScene = scene;
	}

	Ref<Scene> SceneManager::GetActiveScene()
	{
		return s_ActiveScene;
	}

}
