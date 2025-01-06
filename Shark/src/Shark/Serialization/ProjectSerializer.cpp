#include "skpch.h"
#include "ProjectSerializer.h"

#include "Shark/Serialization/YAML.h"
#include "Shark/Serialization/SerializationMacros.h"
#include "Shark/Debug/Profiler.h"

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
		out << YAML::Key << "MaxTimestep" << YAML::Value << config.Physics.MaxTimestep;
		out << YAML::EndMap;

		out << YAML::Key << "Log" << YAML::Value;
		out << YAML::BeginSeq;
		for (const auto& [name, setting] : Log::EnabledTags())
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << name;
			out << YAML::Key << "Enabled" << YAML::Value << setting.Enabled;
			out << YAML::Key << "Level" << YAML::Value << setting.Level;
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;

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
		SK_CORE_TRACE("    MaxTimestep: {}", config.Physics.MaxTimestep);

		return true;
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filePath)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(m_Project);
		if (!m_Project)
			return false;

		SK_CORE_ASSERT(filePath.is_absolute());

		ScopedTimer timer("Deserializing Project");
		YAML::Node fileNode = YAML::LoadFile(filePath);

		auto projectNode = fileNode["Project"];
		if (!projectNode)
			return false;

		auto& config = m_Project->GetConfigMutable();
		config.Name = projectNode["Name"].as<std::string>();
		config.Directory = filePath.parent_path().generic_wstring();
		config.StartupScene = projectNode["StartupScene"].as<AssetHandle>();

		config.AssetsDirectory = projectNode["Assets"].as<std::filesystem::path>();
		config.ScriptModulePath = projectNode["ScriptModulePath"].as<std::string>();

		auto physicsNode = projectNode["Physics"];
		config.Physics.Gravity = physicsNode["Gravity"].as<glm::vec2>();
		config.Physics.VelocityIterations = physicsNode["VelocityIterations"].as<uint32_t>();
		config.Physics.PositionIterations = physicsNode["PositionIterations"].as<uint32_t>();
		config.Physics.FixedTimeStep = physicsNode["FixedTimeStep"].as<float>();
		config.Physics.MaxTimestep = physicsNode["MaxTimestep"].as<float>(0.016f);

		auto logNode = projectNode["Log"];
		if (logNode)
		{
			std::map<std::string, TagSettings> logTags;
			for (auto entryNode : logNode)
			{
				std::string name;
				TagSettings setting;
				SK_DESERIALIZE_PROPERTY(entryNode, "Name", name, "");
				SK_DESERIALIZE_PROPERTY(entryNode, "Enabled", setting.Enabled, true);
				SK_DESERIALIZE_PROPERTY(entryNode, "Level", setting.Level, LogLevel::Trace);
				logTags[name] = setting;
			}

			Log::EnabledTags() = logTags;
		}

		SK_CORE_INFO_TAG("Core", "Deserializing Project from: {}", filePath);
		SK_CORE_TRACE_TAG("Core", "  Name: {}", config.Name);
		SK_CORE_TRACE_TAG("Core", "  Assets Path: {}", config.AssetsDirectory);
		SK_CORE_TRACE_TAG("Core", "  Startup Scene: {}", config.StartupScene);
		SK_CORE_TRACE_TAG("Core", "  Script Module Path: {}", config.ScriptModulePath);
		SK_CORE_TRACE_TAG("Core", "  Physics:");
		SK_CORE_TRACE_TAG("Core", "    Gravity: {}", config.Physics.Gravity);
		SK_CORE_TRACE_TAG("Core", "    ValocityIterations: {}", config.Physics.VelocityIterations);
		SK_CORE_TRACE_TAG("Core", "    PositionIterations: {}", config.Physics.PositionIterations);
		SK_CORE_TRACE_TAG("Core", "    FixedTimeStep: {}", config.Physics.FixedTimeStep);
		SK_CORE_TRACE_TAG("Core", "    MaxTimestep: {}", config.Physics.MaxTimestep);
		return true;
	}

}
