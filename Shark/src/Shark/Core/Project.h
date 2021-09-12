#pragma once

#include <filesystem>
#include "Shark/Utility/Utility.h"

namespace Shark {

	// 232 bytes
	class Project
	{
	public:
		Project();

		bool SaveProjectFile();
		bool LoadProject();

		bool HasStartupScene() const;


		const std::string&           GetProjectName() const                                    { return m_ProjectName; }
		const std::filesystem::path& GetAssetsPath() const                                     { return m_AssetsPath; }
		const std::filesystem::path& GetTexturesPath() const                                   { return m_TexturesPath; }
		const std::filesystem::path& GetScenesPath() const                                     { return m_ScenesPath; }
		const std::filesystem::path& GetStartupScene() const                                   { return m_StartupScene; }
		std::filesystem::path        GetCacheDirectory() const                                 { return m_AssetsPath / "Cache"; }

		std::string&           GetProjectName()                                                { return m_ProjectName; }
		std::filesystem::path& GetAssetsPath()                                                 { return m_AssetsPath; }
		std::filesystem::path& GetTexturesPath()                                               { return m_TexturesPath; }
		std::filesystem::path& GetScenesPath()                                                 { return m_ScenesPath; }
		std::filesystem::path& GetStartupScene()                                               { return m_StartupScene; }
		

		void SetProjectName(const std::string& projName = "Untitled")                          { m_ProjectName = projName; }
		void SetAssetsPath(const std::filesystem::path& assetsPath = "assets")                 { m_AssetsPath = assetsPath; }
		void SetScenePath(const std::filesystem::path& scenesPath = "assets/Scenes")           { m_ScenesPath = scenesPath; }
		void SetTexturesPath(const std::filesystem::path& texturesPath = "assets/Textures")    { m_TexturesPath = texturesPath; }
		void SetStartupScene(const std::filesystem::path& starupScene = "")                    { m_StartupScene = starupScene; }


		uint32_t GetNumScenes() const                                                          { return (uint32_t)m_Scenes.size(); }
		std::filesystem::path& GetSceneAt(uint32_t index)                                      { return m_Scenes[index]; }
		const std::filesystem::path& GetSceneAt(uint32_t index) const                          { return m_Scenes[index]; }
		void AddScene(const std::filesystem::path& filepath)                                   { m_Scenes.emplace_back(filepath); }
		void Remove(uint32_t index)                                                            { Utility::Erase(m_Scenes, index); }
		const std::vector<std::filesystem::path>& GetScenes() const                            { return m_Scenes; }

	private:
		void CreateDefautlProject();

	private:
		std::string m_ProjectName;
		std::filesystem::path m_AssetsPath;
		std::filesystem::path m_TexturesPath;
		std::filesystem::path m_ScenesPath;
		std::filesystem::path m_StartupScene;
		std::vector<std::filesystem::path> m_Scenes;

		friend bool Internal_Serialize(const Project& proj);
		friend bool Internal_DeSerialize(Project& proj);
	};

}
