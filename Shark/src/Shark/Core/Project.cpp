#include "skpch.h"
#include "Project.h"

#include "Shark/Core/Timer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Utility/YAMLUtils.h"

#include "Shark/Debug/Instrumentor.h"

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
		return GetActiveConfig().AssetsDirectory;
	}

	const std::filesystem::path& Project::GetStartupScenePath()
	{
		return GetActiveConfig().StartupScenePath;
	}

	const glm::vec2& Project::GetGravity()
	{
		return GetActiveConfig().Gravity;
	}

	uint32_t Project::GetVelocityIterations()
	{
		return GetActiveConfig().VelocityIterations;
	}

	uint32_t Project::GetPositionIterations()
	{
		return GetActiveConfig().PositionIterations;
	}

	float Project::GetFixedTimeStep()
	{
		return GetActiveConfig().FixedTimeStep;
	}

	std::filesystem::path Project::RelativeCopy(const std::filesystem::path& filePath)
	{
		return FileSystem::FormatDefaultCopy(std::filesystem::relative(filePath, GetActiveConfig().ProjectDirectory));
	}

	std::filesystem::path Project::AbsolueCopy(const std::filesystem::path& filePath)
	{
		return FileSystem::FormatDefaultCopy(GetActiveConfig().ProjectDirectory / filePath);
	}

	void Project::Relative(std::filesystem::path& filePath)
	{
		filePath = std::filesystem::relative(filePath, GetActiveConfig().ProjectDirectory);
		FileSystem::FormatDefault(filePath);
	}

	void Project::Absolue(std::filesystem::path& filePath)
	{
		filePath = GetActiveConfig().ProjectDirectory / filePath;
		FileSystem::FormatDefault(filePath);
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
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(m_Project);
		if (!m_Project)
			return false;

		SK_CORE_ASSERT(filePath.is_absolute());

		Timer timer;

		YAML::Emitter out;

		const auto& config = m_Project->GetConfig();
		const auto assetsPath = std::filesystem::relative(config.AssetsDirectory, config.ProjectDirectory);
		const auto startupScenePath = std::filesystem::relative(config.StartupScenePath, config.ProjectDirectory);
		const auto scriptModulePath = std::filesystem::relative(config.ScriptModulePath, config.ProjectDirectory).string();

		out << YAML::BeginMap;
		out << YAML::Key << "Project" << YAML::Value;

		out << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << config.Name;
		out << YAML::Key << "Assets" << YAML::Value << assetsPath;
		out << YAML::Key << "StartupScene" << YAML::Value << startupScenePath;

		out << YAML::Key << "ScriptModulePath" << YAML::Value << scriptModulePath;

		out << YAML::Key << "Physics" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Gravity" << YAML::Value << config.Gravity;
		out << YAML::Key << "VelocityIterations" << YAML::Value << config.VelocityIterations;
		out << YAML::Key << "PositionIterations" << YAML::Value << config.PositionIterations;
		out << YAML::Key << "FixedTimeStep" << YAML::Value << config.FixedTimeStep;
		out << YAML::EndMap;
		out << YAML::EndMap;

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
		SK_CORE_TRACE("  Assets Path: {}", assetsPath);
		SK_CORE_TRACE("  Startup Scene Path: {}", startupScenePath);
		SK_CORE_TRACE("  Script Module Path: {}", scriptModulePath);
		SK_CORE_TRACE("  Physics:");
		SK_CORE_TRACE("    Gravity: {}", config.Gravity);
		SK_CORE_TRACE("    ValocityIterations: {}", config.VelocityIterations);
		SK_CORE_TRACE("    PositionIterations: {}", config.PositionIterations);
		SK_CORE_TRACE("    FixedTimeStep: {}", config.FixedTimeStep);
		SK_CORE_INFO("Project Serialization tock: {}ms", time.MilliSeconds());

		return true;
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(m_Project);
		if (!m_Project)
			return false;

		SK_CORE_ASSERT(filePath.is_absolute());

		Timer timer;
		YAML::Node in = YAML::LoadFile(filePath);

		auto project = in["Project"];
		if (!project)
			return false;

		auto& config = m_Project->m_Config;
		config.Name             = project["Name"].as<std::string>();
		auto assetsDirectory    = project["Assets"].as<std::filesystem::path>();
		auto startupScenePath   = project["StartupScene"].as<std::filesystem::path>();
		auto scriptModulePath   = project["ScriptModulePath"].as<std::filesystem::path>();

		auto physics = project["Physics"];
		if (physics)
		{
			config.Gravity            = physics["Gravity"].as<glm::vec2>();
			config.VelocityIterations = physics["VelocityIterations"].as<uint32_t>();
			config.PositionIterations = physics["PositionIterations"].as<uint32_t>();
			config.FixedTimeStep      = physics["FixedTimeStep"].as<float>();
		}
		else
		{
			config.Gravity            = { 0.0f, 9.81f };
			config.VelocityIterations = 8;
			config.PositionIterations = 3;
			config.FixedTimeStep      = 0.001f;
		}

		config.ProjectDirectory = FileSystem::FormatDefaultCopy(filePath.parent_path());
		config.AssetsDirectory = FileSystem::FormatDefaultCopy(config.ProjectDirectory / assetsDirectory);
		config.StartupScenePath = FileSystem::FormatDefaultCopy(config.ProjectDirectory / startupScenePath);
		config.ScriptModulePath = FileSystem::FormatDefaultCopy(config.ProjectDirectory / scriptModulePath).string();

		SK_CORE_ASSERT(config.ProjectDirectory.is_absolute());
		SK_CORE_ASSERT(config.AssetsDirectory.is_absolute());
		SK_CORE_ASSERT(config.StartupScenePath.is_absolute());

		TimeStep time = timer.Stop();

		SK_CORE_INFO("Deserializing Project from: {}", filePath);
		SK_CORE_TRACE("  Name: {}", config.Name);
		SK_CORE_TRACE("  Assets Path: {}", assetsDirectory);
		SK_CORE_TRACE("  Startup Scene Path: {}", startupScenePath);
		SK_CORE_TRACE("  Script Module Path: {}", scriptModulePath);
		SK_CORE_TRACE("  Physics:");
		SK_CORE_TRACE("    Gravity: {}", config.Gravity);
		SK_CORE_TRACE("    ValocityIterations: {}", config.VelocityIterations);
		SK_CORE_TRACE("    PositionIterations: {}", config.PositionIterations);
		SK_CORE_TRACE("    FixedTimeStep: {}", config.FixedTimeStep);
		SK_CORE_INFO("Project Deserialization tock: {}ms", time.MilliSeconds());

		return true;
	}

}
