#include "skpch.h"
#include "Project.h"

#include <yaml-cpp/yaml.h>
#include "Shark/Utility/YAMLUtils.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	bool Internal_Serialize(const Project& proj)
	{
		SK_PROFILE_FUNCTION();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "ProjectName"   << YAML::Value << proj.m_ProjectName;
		out << YAML::Key << "AssetsPath"    << YAML::Value << proj.m_AssetsPath;
		out << YAML::Key << "TexturesPath"  << YAML::Value << proj.m_TexturesPath;
		out << YAML::Key << "ScenesPath"    << YAML::Value << proj.m_ScenesPath;
		out << YAML::Key << "StartupScene"  << YAML::Value << proj.m_StartupScene;
		out << YAML::Key << "Scenes";
		out << YAML::BeginSeq;
		for (auto&& scenes : proj.m_Scenes)
			out << scenes;
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout("project.skproj");
		if (!fout)
		{
			SK_CORE_ERROR("Invalid File Path");
			return false;
		}

		fout << out.c_str();
			
		return true;

	}

	bool Internal_DeSerialize(Project& proj)
	{
		SK_PROFILE_FUNCTION();

		YAML::Node in = YAML::LoadFile("project.skproj");
		SK_CORE_ASSERT(in);

		auto projName = in["ProjectName"];
		SK_CORE_ASSERT(projName, "Project Name not found! Use Fallback \"Untitled\"");
		if (!projName) return false;

		auto assetsPath = in["AssetsPath"];
		SK_CORE_ASSERT(assetsPath, "Asset Path not found! Use Fallback \"assets\"");
		if (!assetsPath) return false;

		auto texturesPath = in["TexturesPath"];
		SK_CORE_ASSERT(texturesPath, "Textures Path not found! Use Fallback \"assets/Textures\"");
		if (!texturesPath) return false;

		auto scenesPath = in["ScenesPath"];
		SK_CORE_ASSERT(scenesPath, "Scenes Path not found! use Fallback \"assets/Textures\"");
		if (!scenesPath) return false;

		auto startup = in["StartupScene"];
		SK_CORE_ASSERT(startup, "Starup Scene not found! use Fallback \"\"");
		if (!startup) return false;

		auto scenes = in["Scenes"];
		SK_CORE_ASSERT(scenes, "No Scenes Section found!");
		if (!scenes) return false;


		proj.m_ProjectName = projName.as<std::string>();
		proj.m_AssetsPath = assetsPath.as<std::filesystem::path>();
		proj.m_TexturesPath = texturesPath.as<std::filesystem::path>();
		proj.m_ScenesPath = scenesPath.as<std::filesystem::path>();
		proj.m_StartupScene = startup.as<std::filesystem::path>();

		for (auto&& s : scenes)
			proj.m_Scenes.emplace_back(s.as<std::filesystem::path>());

		SK_CORE_INFO("Project Deserialized");
		SK_CORE_TRACE("Project Name: {}", proj.m_ProjectName);
		SK_CORE_TRACE("Assets Path: {}", proj.m_AssetsPath);
		SK_CORE_TRACE("scenes Path: {}", proj.m_ScenesPath);
		SK_CORE_TRACE("Textures Path: {}", proj.m_TexturesPath);
		SK_CORE_TRACE("Startup Scene: {}", proj.m_StartupScene);
		SK_CORE_TRACE("Scenes: {}", proj.m_Scenes.size());
		uint32_t i = 0;
		for (const auto& s : proj.m_Scenes)
			SK_CORE_TRACE(" - [{}] {}", i++, s);

		return true;
	}


	Project::Project()
	{
		SK_PROFILE_FUNCTION();

		if (!LoadProject())
		{
			CreateDefautlProject();
			SaveProjectFile();
		}
	}

	bool Project::SaveProjectFile()
	{
		SK_PROFILE_FUNCTION();

		return Internal_Serialize(*this);
	}

	bool Project::LoadProject()
	{
		SK_PROFILE_FUNCTION();

		m_Scenes.clear();
		if (FileSystem::Exists(std::filesystem::path("project.skproj")))
			return Internal_DeSerialize(*this);

		SK_CORE_WARN("Project File not found!");
		return false;
	}

	bool Project::HasStartupScene() const
	{
		SK_CORE_ASSERT(m_StartupScene.empty() ? true : FileSystem::Exists(m_StartupScene), "Startup Scene is set but the file dosn't exist!");
		return !m_StartupScene.empty();
	}

	void Project::CreateDefautlProject()
	{
		m_ProjectName = "Untitled";
		m_AssetsPath = "assets";
		m_TexturesPath = std::filesystem::path{};
		m_ScenesPath = std::filesystem::path{};
		m_StartupScene = std::filesystem::path{};
	}

}