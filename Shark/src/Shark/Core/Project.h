#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Asset/AssetTypes.h"

namespace Shark {
	class AssetManagerBase;
	class RuntimeAssetManager;
	class EditorAssetManager;
	class ScriptEngine;
}

namespace Shark {

	struct PhysicsConfig
	{
		glm::vec2 Gravity;
		uint32_t VelocityIterations;
		uint32_t PositionIterations;
		TimeStep FixedTimeStep;
		TimeStep MaxTimestep;
	};

	class ProjectConfig : public RefCount
	{
	public:
		void Rename(const std::string& newName);

		std::string GetProjectFilepath() const { return fmt::format("{}/{}.skproj", Directory.generic_string(), Name); }
		std::string GetScriptModulePath() const { return fmt::format("{}/{}", Directory.generic_string(), ScriptModulePath); }
		std::filesystem::path GetDirectory() const { return Directory; }
		std::filesystem::path GetAssetsDirectory() const { return Directory / AssetsDirectory; }

		std::filesystem::path GetRelative(const std::filesystem::path& path) const;
		std::filesystem::path GetAbsolute(const std::filesystem::path& path) const;

		void CopyTo(Ref<ProjectConfig> config);
	public:
		std::string Name;
		std::filesystem::path Directory;
		std::filesystem::path AssetsDirectory;

		AssetHandle StartupScene;
		std::string ScriptModulePath;

		PhysicsConfig Physics;
	};

	class Project
	{
	public:
		Project();
		~Project();

		static void SetActive(Ref<ProjectConfig> config);
		static void SetActiveRuntime(Ref<ProjectConfig> config);
		static Ref<ProjectConfig> GetActive();

		static Ref<AssetManagerBase> GetAssetManager();
		static Ref<EditorAssetManager> GetEditorAssetManager();
		static Ref<RuntimeAssetManager> GetRuntimeAssetManager();

		static Ref<ScriptEngine> GetScriptEngine();
		static void RestartScriptEngine(bool loadAppAssembly = true);

		static const std::string& GetName() { return s_ActiveConfig->Name; }
		static std::filesystem::path GetProjectFilePath() { return s_ActiveConfig->GetProjectFilepath(); }
		static std::filesystem::path GetActiveDirectory() { return s_ActiveConfig->GetDirectory(); }
		static std::filesystem::path GetActiveAssetsDirectory() { return s_ActiveConfig->GetAssetsDirectory(); }

	private:
		static Ref<AssetManagerBase> s_AssetManager;
		static Ref<ScriptEngine> s_ScriptEngine;
		static Ref<ProjectConfig> s_ActiveConfig;
	};

}
