#include "skpch.h"
#include "Project.h"

#include "Shark/Core/Timer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Utility/YAMLUtils.h"

#include <yaml-cpp/yaml.h>

namespace Shark {

	static Ref<Project> s_ActiveProject = nullptr;

	static const ProjectConfig& GetActiveConfig()
	{
		return Project::GetActive()->GetConfig();
	}

	const std::filesystem::path& Project::GetProjectDirectory()
	{
		return GetActiveConfig().ProjectDirectory;
	}

	const std::filesystem::path& Project::GetAssetsPath()
	{
		return GetActiveConfig().AssetsPath;
	}

	const std::filesystem::path& Project::GetScenesPath()
	{
		return GetActiveConfig().ScenesPath;
	}

	const std::filesystem::path& Project::GetTexturesPath()
	{
		return GetActiveConfig().TexturesPath;
	}

	const std::filesystem::path& Project::GetStartupScenePath()
	{
		return GetActiveConfig().StartupScenePath;
	}

	std::filesystem::path Project::MakeRelative(const std::filesystem::path& filePath)
	{
		return FileSystem::MakeDefaultFormat(std::filesystem::relative(filePath, GetActiveConfig().ProjectDirectory));
	}

	std::filesystem::path Project::MakeAbsolue(const std::filesystem::path& filePath)
	{
		return FileSystem::MakeDefaultFormat(GetActiveConfig().ProjectDirectory / filePath);
	}

	Ref<Project> Project::GetActive()
	{
		return s_ActiveProject;
	}

	void Project::SetActive(Ref<Project> project)
	{
		s_ActiveProject = project;
	}



	ProjectSerializer::ProjectSerializer(Ref<Project> project)
		: m_Project(project)
	{
	}

	bool ProjectSerializer::Serialize(const std::filesystem::path& filePath)
	{
		SK_CORE_ASSERT(m_Project);
		if (!m_Project)
			return false;

		SK_CORE_ASSERT(filePath.is_absolute());

		Timer timer;

		YAML::Emitter out;

		const auto& config = m_Project->GetConfig();
		//SK_CORE_ASSERT(FileSystem::IsRelative(config.AssetsPath, config.ProjectDirectory));
		//SK_CORE_ASSERT(FileSystem::IsRelative(config.ScenesPath, config.ProjectDirectory));
		//SK_CORE_ASSERT(FileSystem::IsRelative(config.TexturesPath, config.ProjectDirectory));
		//SK_CORE_ASSERT(FileSystem::IsRelative(config.StartupScenePath, config.ProjectDirectory));

		out << YAML::BeginMap;

		const auto assetsPath = std::filesystem::relative(config.AssetsPath, config.ProjectDirectory);
		const auto scenesPath = std::filesystem::relative(config.ScenesPath, config.ProjectDirectory);
		const auto texturesPath = std::filesystem::relative(config.TexturesPath, config.ProjectDirectory);
		const auto startupScenePath = std::filesystem::relative(config.StartupScenePath, config.ProjectDirectory);

		out << YAML::Key << "Name" << YAML::Value << config.Name;
		out << YAML::Key << "AssetsPath" << YAML::Value << assetsPath;
		out << YAML::Key << "ScenesPath" << YAML::Value << scenesPath;
		out << YAML::Key << "TexturesPath" << YAML::Value << texturesPath;
		out << YAML::Key << "StartupScenePath" << YAML::Value << startupScenePath;

		out << YAML::EndMap;

		if (!out.good())
		{
			SK_CORE_ERROR("YAML Error: {}", out.GetLastError());
			SK_CORE_ASSERT(false);
			return false;
		}

		std::ofstream fout(filePath);
		SK_CORE_ASSERT(fout, "ofstream flailed to open file");
		if (!fout)
			return false;

		fout << out.c_str();
		TimeStep time = timer.Stop();

		SK_CORE_INFO("Serializing Project To: {}", filePath);
		SK_CORE_TRACE("  Name: {}", config.Name);
		SK_CORE_TRACE("  Assets Path: {}", config.AssetsPath);
		SK_CORE_TRACE("  Scenes Path: {}", config.ScenesPath);
		SK_CORE_TRACE("  Textures Path: {}", config.TexturesPath);
		SK_CORE_TRACE("  Startup Scene Path: {}", config.StartupScenePath);
		SK_CORE_INFO("Project Serialization tock: {}ms", time.MilliSeconds());

		return true;
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filePath)
	{
		SK_CORE_ASSERT(m_Project);
		if (!m_Project)
			return false;

		SK_CORE_ASSERT(filePath.is_absolute());

		Timer timer;
		YAML::Node in = YAML::LoadFile(filePath);

		auto name = in["Name"];
		auto assetsPath = in["AssetsPath"];
		auto scenesPath = in["ScenesPath"];
		auto texturesPath = in["TexturesPath"];
		auto startupScenePath = in["StartupScenePath"];

		SK_CORE_ASSERT(name);
		SK_CORE_ASSERT(assetsPath);
		SK_CORE_ASSERT(scenesPath);
		SK_CORE_ASSERT(texturesPath);
		SK_CORE_ASSERT(startupScenePath);

		auto& config = m_Project->m_Config;
		config.Name = name.as<std::string>();
		config.AssetsPath = assetsPath.as<std::filesystem::path>();
		config.ScenesPath = scenesPath.as<std::filesystem::path>();
		config.TexturesPath = texturesPath.as<std::filesystem::path>();
		config.StartupScenePath = startupScenePath.as<std::filesystem::path>();

		config.ProjectDirectory = FileSystem::MakeDefaultFormat(filePath.parent_path());
		config.ProjectFileName = filePath.filename().string();

		config.AssetsPath = FileSystem::MakeDefaultFormat(config.ProjectDirectory / config.AssetsPath);
		config.ScenesPath = FileSystem::MakeDefaultFormat(config.ProjectDirectory / config.ScenesPath);
		config.TexturesPath = FileSystem::MakeDefaultFormat(config.ProjectDirectory / config.TexturesPath);
		config.StartupScenePath = FileSystem::MakeDefaultFormat(config.ProjectDirectory / config.StartupScenePath);

		SK_CORE_ASSERT(config.ProjectDirectory.is_absolute())
		SK_CORE_ASSERT(config.AssetsPath.is_absolute())
		SK_CORE_ASSERT(config.ScenesPath.is_absolute())
		SK_CORE_ASSERT(config.TexturesPath.is_absolute())
		SK_CORE_ASSERT(config.StartupScenePath.is_absolute())

		TimeStep time = timer.Stop();

		SK_CORE_INFO("Deserializing Project from: {}", filePath);
		SK_CORE_TRACE("  Name: {}", config.Name);
		SK_CORE_TRACE("  Project Dir: {}", config.ProjectDirectory);
		SK_CORE_TRACE("  Assets Path: {}", config.AssetsPath);
		SK_CORE_TRACE("  Scenes Path: {}", config.ScenesPath);
		SK_CORE_TRACE("  Textures Path: {}", config.TexturesPath);
		SK_CORE_TRACE("  Startup Scene Path: {}", config.StartupScenePath);
		SK_CORE_INFO("Project Deserialization tock: {}ms", time.MilliSeconds());

		return true;
	}

}
