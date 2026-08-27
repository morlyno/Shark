#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"

#include <Coral/HostInstance.hpp>

namespace Shark {
	class ProjectConfig;

	class Scene;
	class Entity;

	class ScriptHost;
	class ScriptStorage;
	struct ScriptMetadata;
}

namespace Shark {

	class ScriptEngine : public RefCount
	{
	public:
		ScriptEngine(ScriptHost& host, Ref<ProjectConfig> projectConfig);
		~ScriptEngine();

		// Shortcut for Project::GetScriptEngine()
		static ScriptEngine& Get();

		void LoadAppAssembly();

		bool CoreInitialized() const { return m_CoreAssembly != nullptr; }
		bool AppAssemblyLoaded() const { return m_AppAssembly != nullptr; }

		bool IsValidScriptID(uint64_t scriptID);

		Coral::ManagedObject* Instantiate(UUID entityID, ScriptStorage& storage);
		void Destoy(UUID entityID, ScriptStorage& storage);

	public:
		void SetCurrentScene(Ref<Scene> scene);
		Ref<Scene> GetCurrentSceen() const;

		const ScriptMetadata& GetScriptMetadata(uint64_t scriptID) const;
		uint64_t FindScriptMetadata(std::string_view fullName) const;
		const std::unordered_map<uint64_t, ScriptMetadata>& GetScripts() const;

	private:
		void BuildScriptCache();

	private:
		ScriptHost& m_Host;
		Coral::AssemblyLoadContext m_LoadContext;

		Ref<ProjectConfig> m_ProjectConfig;
		Coral::ManagedAssembly* m_CoreAssembly = nullptr;
		Coral::ManagedAssembly* m_AppAssembly = nullptr;

		Ref<Scene> m_CurrentScene;

		// script id to metadata
		std::unordered_map<uint64_t, ScriptMetadata> m_ScriptMetadata;

		friend class ScriptGlue;
		friend class ScriptEnginePanel;
	};

}
