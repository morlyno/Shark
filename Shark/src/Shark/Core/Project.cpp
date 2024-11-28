#include "skpch.h"
#include "Project.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	void Project::SetActive(Ref<Project> project)
	{
		if (s_ActiveProject)
		{
			s_AssetManager = nullptr;
			ScriptEngine::Get().ShutdownCore();
		}

		s_ActiveProject = project;

		if (project)
		{
			s_AssetManager = Ref<EditorAssetManager>::Create(project);
			ScriptEngine::Get().InitializeCore(project);
		}
	}

	void Project::SetActiveRuntime(Ref<Project> project)
	{
		if (s_ActiveProject)
		{
			s_AssetManager = nullptr;
			ScriptEngine::Get().ShutdownCore();
		}

		s_ActiveProject = project;

		if (project)
		{
			s_AssetManager = Ref<RuntimeAssetManager>::Create();
			ScriptEngine::Get().InitializeCore(project);
		}
	}

	Ref<Project> Project::GetActive()
	{
		return s_ActiveProject;
	}

	std::filesystem::path Project::GetDirectory() const
	{
		return m_Config.Directory;
	}

	std::filesystem::path Project::GetAssetsDirectory() const
	{
		return m_Config.Directory / m_Config.AssetsDirectory;
	}

	std::string Project::GetScriptModulePath() const
	{
		return fmt::format("{}/{}", s_ActiveProject->m_Config.Directory.generic_string(), s_ActiveProject->m_Config.ScriptModulePath);
	}

	std::filesystem::path Project::GetRelative(const std::filesystem::path& path) const
	{
		if (path.is_absolute())
			return std::filesystem::relative(path, m_Config.Directory).lexically_normal().generic_wstring();
		return path.lexically_normal().generic_wstring();
	}

	std::filesystem::path Project::GetAbsolute(const std::filesystem::path& path) const
	{
		return (m_Config.Directory / path).lexically_normal().generic_wstring();
	}

	Project::Project()
	{

	}

	Project::~Project()
	{

	}

	void Project::Rename(const std::string& newName)
	{
		std::filesystem::path fixedName = newName;
		if (FileSystem::GetExtension(newName) != L".skproj")
			FileSystem::ReplaceExtension(fixedName, ".skproj");

		std::filesystem::path projectFile = GetProjectFile();
		if (FileSystem::Rename(projectFile, fixedName.string()))
			m_Config.Name = newName;
	}

}
