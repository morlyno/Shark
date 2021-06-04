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
		*m_SaveState = *m_Active;
	}

	void SceneController::LoadState()
	{
		*m_Active = *m_SaveState;
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