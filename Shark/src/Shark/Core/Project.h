#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/AssetManager/RuntimeAssetManager.h"
#include "Shark/Asset/AssetManager/EditorAssetManager.h"

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
		static const std::filesystem::path& GetActiveDirectory();
		static const std::filesystem::path& GetActiveAssetsDirectory();

		static Ref<AssetManagerBase> GetActiveAssetManager();
		static Ref<RuntimeAssetManager> GetActiveRuntimeAssetManager();
		static Ref<EditorAssetManager> GetActiveEditorAssetManager();

		static void SetActive(Ref<Project> project);
		static Ref<Project> GetActive();

		static Ref<Project> Create(const std::filesystem::path& directory, const std::string& name);
		static Ref<Project> LoadEditor(const std::filesystem::path& filepath);
		static Ref<Project> LoadRuntime(const std::filesystem::path& filepath);
		static bool SaveActive();

	public:
		Project() = default;
		Project(const std::filesystem::path& directory, const std::string& name);
		void Rename(const std::string& newName);

		const std::filesystem::path& GetDirectory() const;
		const std::filesystem::path& GetAssetsDirectory() const;
		std::string GetProjectFilePath() const;

		const ProjectConfig& GetConfig() const;
		ProjectConfig& GetConfigMutable();

		std::filesystem::path GetRelative(const std::filesystem::path& path) const;
		std::filesystem::path GetAbsolute(const std::filesystem::path& path) const;
		void MakeRelative(std::filesystem::path& path) const;
		void MakeAbsolute(std::filesystem::path& path) const;

	public:
		Ref<AssetManagerBase> GetAssetManager() const { return m_AssetManager; }
		Ref<RuntimeAssetManager> GetRuntimeAssetManager() const { return m_AssetManager.As<RuntimeAssetManager>(); }
		Ref<EditorAssetManager> GetEditorAssetManager() const { return m_AssetManager.As<EditorAssetManager>(); }

	private:
		ProjectConfig m_Config;
		Ref<AssetManagerBase> m_AssetManager;
	};


}
