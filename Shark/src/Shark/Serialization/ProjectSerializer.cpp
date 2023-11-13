#include "skpch.h"
#include "ProjectSerializer.h"

#include "Shark/Debug/Profiler.h"

#include "Shark/Utils/YAMLUtils.h"
#include <yaml-cpp/yaml.h>

namespace Shark {

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

		ScopedTimer timer("Project Serialization");

		YAML::Emitter out;

		const auto& config = m_Project->GetConfig();
		const auto assetsPath = m_Project->GetRelative(config.AssetsDirectory);
		const auto scriptModulePath = m_Project->GetRelative(config.ScriptModulePath).string();

		out << YAML::BeginMap;
		out << YAML::Key << "Project" << YAML::Value;

		out << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << config.Name;
		out << YAML::Key << "Assets" << YAML::Value << assetsPath;
		out << YAML::Key << "StartupScene" << YAML::Value << config.StartupScene;

		out << YAML::Key << "ScriptModulePath" << YAML::Value << scriptModulePath;

		out << YAML::Key << "Physics" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "Gravity" << YAML::Value << config.Physics.Gravity;
		out << YAML::Key << "VelocityIterations" << YAML::Value << config.Physics.VelocityIterations;
		out << YAML::Key << "PositionIterations" << YAML::Value << config.Physics.PositionIterations;
		out << YAML::Key << "FixedTimeStep" << YAML::Value << config.Physics.FixedTimeStep;
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
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to create file stream! (Filepath: {0})", filePath);
			return false;
		}

		//fout << out.c_str();
		fout.write(out.c_str(), out.size());

		SK_CORE_INFO("Serializing Project To: {}", filePath);
		SK_CORE_TRACE("  Name: {}", config.Name);
		SK_CORE_TRACE("  Assets Path: {}", assetsPath);
		SK_CORE_TRACE("  Startup Scene: {}", config.StartupScene);
		SK_CORE_TRACE("  Script Module Path: {}", scriptModulePath);
		SK_CORE_TRACE("  Physics:");
		SK_CORE_TRACE("    Gravity: {}", config.Physics.Gravity);
		SK_CORE_TRACE("    ValocityIterations: {}", config.Physics.VelocityIterations);
		SK_CORE_TRACE("    PositionIterations: {}", config.Physics.PositionIterations);
		SK_CORE_TRACE("    FixedTimeStep: {}", config.Physics.FixedTimeStep);

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
		YAML::Node fileNode = YAML::LoadFile(filePath);

		auto projectNode = fileNode["Project"];
		if (!projectNode)
			return false;

		auto& config = m_Project->GetConfigMutable();
		config.Name = projectNode["Name"].as<std::string>();
		config.Directory = filePath.parent_path().generic_wstring();
		config.StartupScene = projectNode["StartupScene"].as<AssetHandle>();

		auto assetsDirectory = projectNode["Assets"].as<std::filesystem::path>();
		auto scriptModule = projectNode["ScriptModulePath"].as<std::string>();
		config.AssetsDirectory = fmt::format("{}/{}", config.Directory, assetsDirectory);
		config.ScriptModulePath = fmt::format("{}/{}", config.Directory, scriptModule);

		auto physicsNode = projectNode["Physics"];
		config.Physics.Gravity = physicsNode["Gravity"].as<glm::vec2>();
		config.Physics.VelocityIterations = physicsNode["VelocityIterations"].as<uint32_t>();
		config.Physics.PositionIterations = physicsNode["PositionIterations"].as<uint32_t>();
		config.Physics.FixedTimeStep = physicsNode["FixedTimeStep"].as<float>();

		float time = timer.ElapsedMilliSeconds();

		SK_CORE_INFO("Deserializing Project from: {}", filePath);
		SK_CORE_TRACE("  Name: {}", config.Name);
		SK_CORE_TRACE("  Assets Path: {}", assetsDirectory);
		SK_CORE_TRACE("  Startup Scene: {}", config.StartupScene);
		SK_CORE_TRACE("  Script Module Path: {}", scriptModule);
		SK_CORE_TRACE("  Physics:");
		SK_CORE_TRACE("    Gravity: {}", config.Physics.Gravity);
		SK_CORE_TRACE("    ValocityIterations: {}", config.Physics.VelocityIterations);
		SK_CORE_TRACE("    PositionIterations: {}", config.Physics.PositionIterations);
		SK_CORE_TRACE("    FixedTimeStep: {}", config.Physics.FixedTimeStep);
		SK_CORE_INFO("Project Deserialization took: {:.4f}ms", time);

		return true;
	}

}
