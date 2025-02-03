#include "skpch.h"
#include "Project.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Project Config ///////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	void ProjectConfig::Rename(const std::string& newName)
	{
		std::filesystem::path fixedName = newName;
		if (FileSystem::GetExtension(newName) != L".skproj")
			FileSystem::ReplaceExtension(fixedName, ".skproj");

		std::filesystem::path projectFile = GetProjectFilepath();
		if (FileSystem::Rename(projectFile, fixedName.string()))
			Name = newName;
	}

	std::filesystem::path ProjectConfig::GetRelative(const std::filesystem::path& path) const
	{
		if (path.is_absolute())
			return std::filesystem::relative(path, Directory).lexically_normal().generic_wstring();
		return path.lexically_normal().generic_wstring();
	}

	std::filesystem::path ProjectConfig::GetAbsolute(const std::filesystem::path& path) const
	{
		return (Directory / path).lexically_normal().generic_wstring();
	}

	void ProjectConfig::CopyTo(Ref<ProjectConfig> config)
	{
		config->Name = Name;
		config->Directory = Directory;
		config->AssetsDirectory = AssetsDirectory;
		config->StartupScene = StartupScene;
		config->ScriptModulePath = ScriptModulePath;
		config->Physics = Physics;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	//// Project //////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	Project::Project()
	{

	}

	Project::~Project()
	{

	}

	void Project::SetActive(Ref<ProjectConfig> config)
	{
		if (s_ActiveConfig)
		{
			s_AssetManager = nullptr;
			s_ScriptEngine = nullptr;
		}

		s_ActiveConfig = config;

		if (config)
		{
			auto& app = Application::Get();
			s_AssetManager = Ref<EditorAssetManager>::Create(config);
			s_ScriptEngine = Ref<ScriptEngine>::Create(app.GetScriptHost(), config);
		}
	}

	void Project::SetActiveRuntime(Ref<ProjectConfig> config)
	{
		if (s_ActiveConfig)
		{
			s_AssetManager = nullptr;
			s_ScriptEngine = nullptr;
		}

		s_ActiveConfig = config;

		if (config)
		{
			auto& app = Application::Get();
			s_AssetManager = Ref<RuntimeAssetManager>::Create();
			s_ScriptEngine = Ref<ScriptEngine>::Create(app.GetScriptHost(), config);
		}
	}

	Ref<ProjectConfig> Project::GetActive()
	{
		return s_ActiveConfig;
	}

	void Project::RestartScriptEngine(bool loadAppAssembly)
	{
		auto& app = Application::Get();

		s_ScriptEngine = nullptr;
		s_ScriptEngine = Ref<ScriptEngine>::Create(app.GetScriptHost(), s_ActiveConfig);
		if (loadAppAssembly)
			s_ScriptEngine->LoadAppAssembly();
	}

}
