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
		static const std::filesystem::path& GetProjectDirectory();

		static const std::filesystem::path& GetAssetsPath();
		static const std::filesystem::path& GetScenesPath();
		static const std::filesystem::path& GetTexturesPath();

		static const std::filesystem::path& GetStartupScenePath();

		static std::filesystem::path MakeRelative(const std::filesystem::path& filePath);
		static std::filesystem::path MakeAbsolue(const std::filesystem::path& filePath);

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
