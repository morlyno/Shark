#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Project.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scripting/ScriptStorage.h"
#include "Shark/Scripting/ScriptTypes.h"

#include <Coral/HostInstance.hpp>

namespace Shark {

	class Scene;

	class ScriptEngine
	{
	public:
		ScriptEngine() = default;
		~ScriptEngine() = default;

		// Shortcut for Application::Get().GetScriptEngine()
		static ScriptEngine& Get();

		void InitializeHost();
		void ShutdownHost();

		void InitializeCore(Ref<ProjectConfig> projectConfig);
		void ShutdownCore();
		void LoadAppAssembly();
		void ReloadAssemblies();

		bool HostInitialized() const { return m_Host != nullptr; }
		bool CoreInitialized() const { return m_CoreAssembly != nullptr; }
		bool AppAssemblyLoaded() const { return m_AppAssembly != nullptr; }

		bool IsValidScriptID(uint64_t scriptID);

		Coral::ManagedObject* Instantiate(UUID entityID, ScriptStorage& storage);
		void Destoy(UUID entityID, ScriptStorage& storage);

	public:
		void SetCurrentScene(Ref<Scene> scene) { m_CurrentScene = scene; }
		Ref<Scene> GetCurrentSceen() const { return m_CurrentScene; }

		const ScriptMetadata& GetScriptMetadata(uint64_t scriptID) const { return m_ScriptMetadata.at(scriptID); }
		uint64_t FindScriptMetadata(std::string_view fullName) const;
		const std::unordered_map<uint64_t, ScriptMetadata>& GetScripts() const { return m_ScriptMetadata; }

	private:
		void BuildScriptCache();

	private:
		Scope<Coral::HostInstance> m_Host;
		Scope<Coral::AssemblyLoadContext> m_LoadContext;

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
