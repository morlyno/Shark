#include "skpch.h"
#include "Project.h"

#include "Shark/Serialization/ProjectSerializer.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	static Ref<Project> s_ActiveProject = nullptr;

	const std::filesystem::path& Project::GetActiveDirectory()
	{
		SK_CORE_VERIFY(s_ActiveProject);
		return s_ActiveProject->GetDirectory();
	}

	const std::filesystem::path& Project::GetActiveAssetsDirectory()
	{
		SK_CORE_VERIFY(s_ActiveProject);
		return s_ActiveProject->GetAssetsDirectory();
	}

	Ref<AssetManagerBase> Project::GetActiveAssetManager()
	{
		SK_CORE_VERIFY(s_ActiveProject);
		return s_ActiveProject->GetAssetManager();
	}

	Ref<RuntimeAssetManager> Project::GetActiveRuntimeAssetManager()
	{
		SK_CORE_VERIFY(s_ActiveProject);
		return s_ActiveProject->GetRuntimeAssetManager();
	}

	Ref<EditorAssetManager> Project::GetActiveEditorAssetManager()
	{
		SK_CORE_VERIFY(s_ActiveProject);
		return s_ActiveProject->GetEditorAssetManager();
	}

	void Project::SetActive(Ref<Project> project)
	{
		s_ActiveProject = project;
	}

	Ref<Project> Project::GetActive()
	{
		return s_ActiveProject;
	}

	Ref<Project> Project::Create(const std::filesystem::path& directory, const std::string& name)
	{
		return Ref<Project>::Create(directory, name);
	}

	Ref<Project> Project::LoadEditor(const std::filesystem::path& filepath)
	{
		auto project = Ref<Project>::Create();
		ProjectSerializer serializer(project);
		if (!serializer.Deserialize(filepath))
			return nullptr;

		project->m_AssetManager = Ref<EditorAssetManager>::Create(project);
		project->m_AssetThread = Ref<EditorAssetThread>::Create();
		return project;
	}

	Ref<Project> Project::LoadRuntime(const std::filesystem::path& filepath)
	{
		auto project = Ref<Project>::Create();
		ProjectSerializer serializer(project);
		if (!serializer.Deserialize(filepath))
			return nullptr;

		project->m_AssetManager = Ref<RuntimeAssetManager>::Create();
		project->m_AssetThread = nullptr; // TODO(moro): RuntimeAssetThread
		return project;
	}

	bool Project::SaveActive()
	{
		ProjectSerializer serializer(s_ActiveProject);
		return serializer.Serialize(s_ActiveProject->GetProjectFilePath());
	}

	const std::filesystem::path& Project::GetDirectory() const
	{
		return m_Config.Directory;
	}

	const std::filesystem::path& Project::GetAssetsDirectory() const
	{
		return m_Config.AssetsDirectory;
	}

	const std::string Project::GetProjectFilePath() const
	{
		return fmt::format("{}/{}.skproj", m_Config.Directory, m_Config.Name);
	}

	const ProjectConfig& Project::GetConfig() const
	{
		return m_Config;
	}

	ProjectConfig& Project::GetConfigMutable()
	{
		return m_Config;
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

	void Project::MakeRelative(std::filesystem::path& path) const
	{
		path = GetRelative(path);
	}

	void Project::MakeAbsolute(std::filesystem::path& path) const
	{
		path = GetAbsolute(path);
	}

	Project::Project(const std::filesystem::path& directory, const std::string& name)
	{
		m_Config.Name = name;
		m_Config.Directory = directory.generic_wstring();
		m_Config.AssetsDirectory = fmt::format("{0}/Assets", m_Config.Directory);
		m_Config.ScriptModulePath = fmt::format("{0}/Binaries/{1}.dll", m_Config.Directory, m_Config.Name);
		m_Config.Physics.Gravity = { 0.0f, -9.81f };
		m_Config.Physics.VelocityIterations = 8;
		m_Config.Physics.PositionIterations = 3;
		m_Config.Physics.FixedTimeStep = 0.001f;

		m_AssetManager = Ref<EditorAssetManager>::Create(this);
	}

	void Project::Rename(const std::string& newName)
	{
		std::filesystem::path projectFile = GetProjectFilePath();
		if (FileSystem::Rename(projectFile, newName))
			m_Config.Name = newName;
	}

}
