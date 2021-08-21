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

		void SetSavePath(const std::string& savepath) { m_SavePath = savepath; }
		const std::string GetSavePath() const { return m_SavePath; }

		bool Serialize() { return Serialize(m_SavePath); }
		bool Serialize(const std::string& filepath);
		bool Deserialize(const std::string& filepath);

		Ref<Scene> Get() const { return m_Active; }
		Ref<Scene> GetSaveState() const { return m_SaveState; }

		const Ref<Scene>& operator->() const { return m_Active; }
		const Ref<Scene>& operator*() const { return m_Active; }
		operator Ref<Scene>() const { return m_Active; }

		operator bool() const { return m_Active.operator bool(); }
	private:
		std::string m_SavePath;
		Ref<Scene> m_Active;
		Ref<Scene> m_SaveState;
	};

}
