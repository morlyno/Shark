#pragma once

#include "Shark/Scene/Scene.h"

namespace Shark {

	class SceneController
	{
	public:
		SceneController() = default;
		SceneController(const Ref<Scene>& scene);
		void SetScene(const Ref<Scene>& scene);

		void SaveState();
		void LoadState();

		bool Serialize() { return Serialize(m_Active->GetFilePath()); }
		bool Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);

		Ref<Scene> Get() const { return m_Active; }
		Ref<Scene> GetSaveState() const { return m_SaveState; }

		const Ref<Scene>& operator->() const { return m_Active; }
		const Ref<Scene>& operator*() const { return m_Active; }
		operator Ref<Scene>() const { return m_Active; }

		operator bool() const { return m_Active.operator bool(); }
	private:
		Ref<Scene> m_Active;
		Ref<Scene> m_SaveState;
	};

}
