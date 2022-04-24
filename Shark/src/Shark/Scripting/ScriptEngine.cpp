#include "skpch.h"
#include "ScriptEngine.h"

#include "Shark/Core/Project.h"
#include "Shark/Scripting/InternalCalls.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/image.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/class.h>
#include <mono/utils/mono-logger.h>
#include <mono/utils/mono-publib.h>

namespace Shark {

	struct ScriptingData
	{
		MonoDomain* RootDomain = nullptr;
		MonoDomain* RuntimeDomain = nullptr;

		MonoAssembly* CoreAssembly = nullptr;
		MonoImage* CoreImage = nullptr;

		MonoAssembly* ScriptAssembly = nullptr;
		MonoImage* ScriptImage = nullptr;

		std::string CoreAssemblyPath;
		std::string ScriptAssemblyPath;
	};
	static ScriptingData s_ScriptData;

	bool ScriptEngine::Init(const std::string& scriptinCoreAssemblyPath)
	{
		s_ScriptData.CoreAssemblyPath = scriptinCoreAssemblyPath;
		mono_set_dirs(SK_CONNECT_TO_STRING(MONO_DIRECTORY, /lib), SK_CONNECT_TO_STRING(MONO_DIRECTORY, /etc));

		s_ScriptData.RootDomain = mono_jit_init("Core Domain");
		SK_CORE_ASSERT(s_ScriptData.RootDomain, "Failed to initialize Domain");

		return true;
	}

	void ScriptEngine::Shutdown()
	{
		if (s_ScriptData.RootDomain)
		{
			mono_jit_cleanup(s_ScriptData.RootDomain);
			s_ScriptData.RootDomain = nullptr;
			s_ScriptData.RuntimeDomain = nullptr;
			s_ScriptData.CoreAssembly = nullptr;
			s_ScriptData.CoreImage = nullptr;
			s_ScriptData.ScriptAssembly = nullptr;
			s_ScriptData.ScriptImage = nullptr;
			s_ScriptData.CoreAssemblyPath.clear();
			s_ScriptData.ScriptAssemblyPath.clear();
		}
	}

	bool ScriptEngine::LoadAssembly(const std::string& assemblyPath)
	{
		CopyScriptModule();

		// Create Runtime Domain
		s_ScriptData.RuntimeDomain = mono_domain_create_appdomain("Script Domain", nullptr);
		SK_CORE_ASSERT(s_ScriptData.RuntimeDomain);
		mono_domain_set(s_ScriptData.RuntimeDomain, 0);

		// Core
		s_ScriptData.CoreAssembly = mono_domain_assembly_open(s_ScriptData.RuntimeDomain, s_ScriptData.CoreAssemblyPath.c_str());
		SK_CORE_ASSERT(s_ScriptData.CoreAssembly);

		s_ScriptData.CoreImage = mono_assembly_get_image(s_ScriptData.CoreAssembly);
		SK_CORE_ASSERT(s_ScriptData.CoreImage);

		MonoCalls::RegsiterInternalCalls();

		// Scripting
		s_ScriptData.ScriptAssemblyPath = assemblyPath;
		s_ScriptData.ScriptAssembly = mono_domain_assembly_open(s_ScriptData.RuntimeDomain, assemblyPath.c_str());
		SK_CORE_ASSERT(s_ScriptData.ScriptAssembly);

		s_ScriptData.ScriptImage = mono_assembly_get_image(s_ScriptData.ScriptAssembly);
		SK_CORE_ASSERT(s_ScriptData.ScriptImage);

		return true;
	}

	void ScriptEngine::UnloadAssembly()
	{
		SK_CORE_ASSERT(s_ScriptData.RootDomain != s_ScriptData.RuntimeDomain);

		mono_domain_set(s_ScriptData.RootDomain, 0);
		mono_domain_unload(s_ScriptData.RuntimeDomain);

		s_ScriptData.RuntimeDomain = nullptr;
		s_ScriptData.CoreAssembly = nullptr;
		s_ScriptData.CoreImage = nullptr;
		s_ScriptData.ScriptAssembly = nullptr;
		s_ScriptData.ScriptImage = nullptr;
	}

	bool ScriptEngine::ReloadAssembly()
	{
		UnloadAssembly();
		LoadAssembly(s_ScriptData.ScriptAssemblyPath);

		return true;
	}

	bool ScriptEngine::CopyScriptModule()
	{
#if SK_DEBUG
		std::filesystem::path binaryOutputDir = Project::GetProjectDirectory() / "bin/Debug-windows-x86_64";
#elif SK_RELEASE
		std::filesystem::path binaryOutputDir = Project::GetProjectDirectory() / "bin/Release-windows-x86_64";
#else
		static_assert(false);
#endif
		std::filesystem::path projectName = Project::GetActive()->GetConfig().Name;
		binaryOutputDir /= projectName / projectName;
		binaryOutputDir.replace_extension(".dll");

		std::filesystem::path scriptModulePath = Project::GetActive()->GetConfig().ScriptModulePath;
		std::filesystem::path scriptModuleDir = scriptModulePath.parent_path();

		if (!std::filesystem::exists(scriptModuleDir))
			std::filesystem::create_directories(scriptModuleDir);

		SK_CORE_ASSERT(std::filesystem::exists(binaryOutputDir));
		SK_CORE_ASSERT(std::filesystem::exists(scriptModuleDir));

		std::error_code err;
		bool succeded = std::filesystem::copy_file(binaryOutputDir, scriptModulePath, std::filesystem::copy_options::overwrite_existing, err);
		SK_CORE_ASSERT(!err, err.message());
		return succeded;
	}

}

