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

	const std::filesystem::path& Project::GetAssetsPathRelative()
	{
		return GetActiveConfig().AssetsPath;
	}

	const std::filesystem::path& Project::GetScenesPathRelative()
	{
		return GetActiveConfig().ScenesPath;
	}

	const std::filesystem::path& Project::GetTexturesPathRelative()
	{
		return GetActiveConfig().TexturesPath;
	}

	std::filesystem::path Project::GetAssetsPathAbsolute()
	{
		return GetActiveConfig().ProjectDirectory.native() + L'/' + GetAssetsPathRelative().native();
	}

	std::filesystem::path Project::GetScenesPathAbsolute()
	{
		return GetActiveConfig().ProjectDirectory.native() + L'/' + GetScenesPathRelative().native();
	}

	std::filesystem::path Project::GetTexturesPathAbsolute()
	{
		return GetActiveConfig().ProjectDirectory.native() + L'/' + GetTexturesPathRelative().native();
	}

	std::filesystem::path Project::GetStartupScenePathAbsolute()
	{
		return GetActiveConfig().ProjectDirectory.native() + L'/' + GetStartupScenePathRelative().native();
	}

	std::filesystem::path Project::GetStartupScenePathRelative()
	{
		return GetActiveConfig().StartupScenePath;
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
		SK_CORE_ASSERT(FileSystem::IsRelative(config.AssetsPath, config.ProjectDirectory));
		SK_CORE_ASSERT(FileSystem::IsRelative(config.ScenesPath, config.ProjectDirectory));
		SK_CORE_ASSERT(FileSystem::IsRelative(config.TexturesPath, config.ProjectDirectory));
		//SK_CORE_ASSERT(FileSystem::IsRelative(config.StartupScenePath, config.ProjectDirectory));

		out << YAML::BeginMap;

		out << YAML::Key << "Name" << YAML::Value << config.Name;
		out << YAML::Key << "AssetsPath" << YAML::Value << config.AssetsPath;
		out << YAML::Key << "ScenesPath" << YAML::Value << config.ScenesPath;
		out << YAML::Key << "TexturesPath" << YAML::Value << config.TexturesPath;
		out << YAML::Key << "StartupScenePath" << YAML::Value << config.StartupScenePath;

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

		SK_CORE_ASSERT(config.ProjectDirectory.is_absolute())
		SK_CORE_ASSERT(FileSystem::IsRelative(config.AssetsPath, config.ProjectDirectory));
		SK_CORE_ASSERT(FileSystem::IsRelative(config.ScenesPath, config.ProjectDirectory));
		SK_CORE_ASSERT(FileSystem::IsRelative(config.TexturesPath, config.ProjectDirectory));

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
