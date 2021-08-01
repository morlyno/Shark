#include "skpch.h"
#include "SceneController.h"

#include "Shark/Scene/SceneSerialization.h"
#include "Shark/Utility/PlatformUtils.h"

namespace Shark {

	SceneController::SceneController(const Ref<Scene>& scene)
		: m_Active(scene), m_SaveState(Ref<Scene>::Create())
	{
		SaveState();
	}

	void SceneController::SetScene(const Ref<Scene>& scene)
	{
		m_Active = scene;
		SaveState();
	}

	void SceneController::SaveState()
	{
		m_Active->Copy(m_SaveState);
	}

	void SceneController::LoadState()
	{
		m_SaveState->Copy(m_Active);
	}

	bool SceneController::Serialize(const std::string& filepath)
	{
		SetSavePath(filepath);
		SceneSerializer serializer(m_Active);
		return serializer.Serialize(filepath);
	}

	bool SceneController::Deserialize(const std::string& filepath)
	{
		m_Active = Ref<Scene>::Create();
		SceneSerializer serializer(m_Active);
		bool succeeded = serializer.Deserialize(filepath);
		SaveState();
		return succeeded;
	}

}