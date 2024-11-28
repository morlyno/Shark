#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/AssetManager/EditorAssetManager.h"
#include "Shark/Asset/AssetManager/RuntimeAssetManager.h"

namespace Shark {

	struct PhysicsConfig
	{
		glm::vec2 Gravity;
		uint32_t VelocityIterations;
		uint32_t PositionIterations;
		float FixedTimeStep;
	};

	struct ProjectConfig
	{
		std::string Name;
		std::filesystem::path Directory;
		std::filesystem::path AssetsDirectory;

		AssetHandle StartupScene;
		std::string ScriptModulePath;

		PhysicsConfig Physics;
	};

	class Project : public RefCount
	{
	public:
		Project();
		~Project();

		ProjectConfig& GetConfigMutable() { return m_Config; }
		const ProjectConfig& GetConfig() const { return m_Config; }

		static void SetActive(Ref<Project> project);
		static void SetActiveRuntime(Ref<Project> project);
		static Ref<Project> GetActive();

		static Ref<AssetManagerBase> GetActiveAssetManager() { return s_AssetManager; }
		static Ref<EditorAssetManager> GetActiveEditorAssetManager() { return s_AssetManager.As<EditorAssetManager>(); }
		static Ref<RuntimeAssetManager> GetActiveRuntimeAssetManager() { return s_AssetManager.As<RuntimeAssetManager>(); }

		Ref<AssetManagerBase> GetAssetManager() { return GetActiveAssetManager(); }
		Ref<EditorAssetManager> GetEditorAssetManager() { return GetActiveEditorAssetManager(); }
		Ref<RuntimeAssetManager> GetRuntimeAssetManager() { return GetActiveRuntimeAssetManager(); }

		void Rename(const std::string& newName);
		std::string GetProjectFile() const { return fmt::format("{}/{}.skproj", m_Config.Directory, m_Config.Name); }
		static std::filesystem::path GetProjectFilePath() { return s_ActiveProject->GetProjectFile(); }

		std::filesystem::path GetDirectory() const;
		std::filesystem::path GetAssetsDirectory() const;
		static std::filesystem::path GetActiveDirectory() { return s_ActiveProject->GetDirectory(); }
		static std::filesystem::path GetActiveAssetsDirectory() { return s_ActiveProject->GetAssetsDirectory(); }
		std::string GetScriptModulePath() const;

		std::filesystem::path GetRelative(const std::filesystem::path& path) const;
		std::filesystem::path GetAbsolute(const std::filesystem::path& path) const;
	private:
		ProjectConfig m_Config;

		static inline Ref<AssetManagerBase> s_AssetManager;
		static inline Ref<Project> s_ActiveProject;
	};


}
