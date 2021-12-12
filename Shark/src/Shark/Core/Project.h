#pragma once

#include "Shark/Core/Base.h"

namespace Shark {

	struct ProjectConfig
	{
		// Name without extention
		std::string Name;

		std::string ProjectFileName;
		std::filesystem::path ProjectDirectory;

		std::filesystem::path AssetsPath;
		std::filesystem::path ScenesPath;
		std::filesystem::path TexturesPath;

		std::filesystem::path StartupScenePath;
	};

	class Project : public RefCount
	{
	public:
		static const std::filesystem::path& GetAssetsPathRelative();
		static const std::filesystem::path& GetScenesPathRelative();
		static const std::filesystem::path& GetTexturesPathRelative();

		static std::filesystem::path GetAssetsPathAbsolute();
		static std::filesystem::path GetScenesPathAbsolute();
		static std::filesystem::path GetTexturesPathAbsolute();

		static std::filesystem::path GetStartupScenePathAbsolute();
		static std::filesystem::path GetStartupScenePathRelative();

		static Ref<Project> GetActive();
		static void SetActive(Ref<Project> project);

	public:
		ProjectConfig& GetConfig() { return m_Config; }
		const ProjectConfig& GetConfig() const { return m_Config; }

	private:
		ProjectConfig m_Config;

		friend class ProjectSerializer;
	};



	class ProjectSerializer
	{
	public:
		ProjectSerializer(Ref<Project> project);
		~ProjectSerializer() = default;

		bool Serialize(const std::filesystem::path& filePath);
		bool Deserialize(const std::filesystem::path& filePath);
	private:
		Ref<Project> m_Project;
	};

}
