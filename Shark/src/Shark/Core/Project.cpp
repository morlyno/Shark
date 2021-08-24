#include "skpch.h"
#include "Project.h"

#include <yaml-cpp/yaml.h>
#include "Shark/Utility/YAMLUtils.h"
#include "Shark/Utility/FileSystem.h"

namespace Shark {

	bool Internal_Serialize(const Project& proj)
	{
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
		YAML::Node in = YAML::LoadFile("project.skproj");
		SK_CORE_ASSERT(in);
		if (!in)
			return false;

		auto projName = in["ProjectName"];
		SK_CORE_ASSERT(projName);
		if (!projName)
			return false;

		auto assetsPath = in["AssetsPath"];
		SK_CORE_ASSERT(assetsPath);
		if (!assetsPath)
			return false;

		auto texturesPath = in["TexturesPath"];
		SK_CORE_ASSERT(texturesPath);
		if (!texturesPath)
			return false;

		auto scenesPath = in["ScenesPath"];
		SK_CORE_ASSERT(scenesPath);
		if (!scenesPath)
			return false;

		auto startup = in["StartupScene"];
		SK_CORE_ASSERT(startup);
		if (!startup)
			return false;

		auto scenes = in["Scenes"];
		SK_CORE_ASSERT(scenes);
		if (!scenes)
			return false;


		proj.m_ProjectName = projName.as<std::string>();
		proj.m_AssetsPath = assetsPath.as<std::filesystem::path>();
		proj.m_TexturesPath = texturesPath.as<std::filesystem::path>();
		proj.m_ScenesPath = scenesPath.as<std::filesystem::path>();
		proj.m_StartupScene = startup.as<std::filesystem::path>();

		for (auto&& s : scenes)
			proj.m_Scenes.emplace_back(s.as<std::filesystem::path>());


		return true;
	}


	Project::Project()
	{
		if (!LoadProject())
		{
			SK_CORE_WARN("Load Project Failed! Creating Standart Projet");
			CreateStandartProject();
		}
	}

	bool Project::SaveProjectFile()
	{
		return Internal_Serialize(*this);
	}

	bool Project::LoadProject()
	{
		m_Scenes.clear();
		if (!FileSystem::Exists(std::filesystem::path("project.skproj")))
		{
			SK_CORE_INFO("Project File not found");
			return false;
		}
		return Internal_DeSerialize(*this);
	}

	bool Project::HasStartupScene() const
	{
		SK_CORE_ASSERT(m_StartupScene.empty() ? true : FileSystem::Exists(m_StartupScene), "Startup Scene dosn't exist");
		return !m_StartupScene.empty();
	}

	void Project::CreateStandartProject()
	{
		m_ProjectName = "Untiled";
		m_AssetsPath = "assets";
		m_TexturesPath = "assets/Textures";
		m_ScenesPath = "assets/Scenes";
		m_Scenes.clear();
	}

}
