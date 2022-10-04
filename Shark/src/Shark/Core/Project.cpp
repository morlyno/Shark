#include "skpch.h"
#include "Project.h"

#include "Shark/Core/Timer.h"
#include "Shark/Utils/String.h"
#include "Shark/Utils/YAMLUtils.h"

#include "Shark/Debug/Instrumentor.h"

#include <yaml-cpp/yaml.h>

namespace Shark {

	static Ref<ProjectInstance> s_ActiveProject = nullptr;

	std::filesystem::path ProjectInstance::GetRelative(const std::filesystem::path& filePath)
	{
		if (filePath.is_absolute())
			return String::FormatDefaultCopy(std::filesystem::relative(filePath, Directory));
		return filePath;
	}

	std::filesystem::path ProjectInstance::GetAbsolue(const std::filesystem::path& filePath)
	{
		return String::FormatDefaultCopy(Directory / filePath);
	}


	const std::string& Project::GetName()
	{
		return s_ActiveProject->Name;
	}

	const std::filesystem::path& Project::GetDirectory()
	{
		return s_ActiveProject->Directory;
	}

	const std::filesystem::path& Project::GetAssetsPath()
	{
		return s_ActiveProject->AssetsDirectory;
	}

	std::filesystem::path Project::RelativeCopy(const std::filesystem::path& filePath)
	{
		return s_ActiveProject->GetRelative(filePath);
	}

	std::filesystem::path Project::AbsolueCopy(const std::filesystem::path& filePath)
	{
		return s_ActiveProject->GetAbsolue(filePath);
	}

	void Project::Relative(std::filesystem::path& filePath)
	{
		filePath = s_ActiveProject->GetRelative(filePath);
	}

	void Project::Absolue(std::filesystem::path& filePath)
	{
		filePath = s_ActiveProject->GetAbsolue(filePath);
	}

	Ref<ProjectInstance> Project::GetActive()
	{
		return s_ActiveProject;
	}

	void Project::SetActive(Ref<ProjectInstance> project)
	{
		s_ActiveProject = project;
	}



	ProjectSerializer::ProjectSerializer(Ref<ProjectInstance> project)
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

		const auto& config = *m_Project;
		const auto assetsPath = std::filesystem::relative(config.AssetsDirectory, config.Directory);
		const auto startupScenePath = std::filesystem::relative(config.StartupScenePath, config.Directory);
		const auto scriptModulePath = std::filesystem::relative(config.ScriptModulePath, config.Directory).string();

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
			SK_CORE_ERROR_TAG("Serialization", "Failed to serialize project! {0}", out.GetLastError());
			SK_CORE_ASSERT(false);
			return false;
		}

		std::ofstream fout(filePath);
		if (!fout)
			return false;

		fout << out.c_str();
		float time = timer.ElapsedMilliSeconds();

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
		SK_CORE_INFO("Project Serialization took: {:.4f}ms", time);

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

		auto& config = *m_Project;
		config.Name             = project["Name"].as<std::string>();
		auto assetsDirectory    = project["Assets"].as<std::filesystem::path>();
		auto startupScenePath   = project["StartupScene"].as<std::filesystem::path>();
		auto scriptModulePath   = project["ScriptModulePath"].as<std::filesystem::path>();

		auto physics = project["Physics"];
		config.Gravity            = physics["Gravity"].as<glm::vec2>(glm::vec2(0.0f, 9.81f));
		config.VelocityIterations = physics["VelocityIterations"].as<uint32_t>(8);
		config.PositionIterations = physics["PositionIterations"].as<uint32_t>(3);
		config.FixedTimeStep      = physics["FixedTimeStep"].as<float>(0.001f);

		config.Directory = String::FormatDefaultCopy(filePath.parent_path());
		config.AssetsDirectory = String::FormatDefaultCopy(config.Directory / assetsDirectory);
		config.StartupScenePath = String::FormatDefaultCopy(config.Directory / startupScenePath);
		config.ScriptModulePath = String::FormatDefaultCopy(config.Directory / scriptModulePath).string();

		SK_CORE_ASSERT(config.Directory.is_absolute());
		SK_CORE_ASSERT(config.AssetsDirectory.is_absolute());
		SK_CORE_ASSERT(config.StartupScenePath.is_absolute());

		float time = timer.ElapsedMilliSeconds();

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
		SK_CORE_INFO("Project Deserialization took: {:.4f}ms", time);

		return true;
	}

}
