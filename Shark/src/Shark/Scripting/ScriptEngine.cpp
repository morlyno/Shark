#include "skpch.h"
#include "ScriptEngine.h"

#include "Shark/Core/TimeStep.h"
#include "Shark/Core/Application.h"
#include "Shark/Core/Project.h"

#include "Shark/Scene/Components/ScriptComponent.h"

#include "Shark/Scripting/MonoGlue.h"

#include "Shark/Utils/String.h"

#include "Shark/Debug/Instrumentor.h"

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

		bool AssemblyLoaded = false;
	};
	static ScriptingData s_ScriptData;

	static Ref<Scene> s_ActiveScene = nullptr;

	namespace utils {

		std::filesystem::path GetScriptingCoreOuputPath()
		{
#if SK_DEBUG
			return std::filesystem::current_path() / "../bin/Debug-windows-x86_64/ScriptingCore/ScriptingCore.dll";
#elif SK_RELEASE
			return std::filesystem::current_path() / "../bin/Release-windows-x86_64/ScriptingCore/ScriptingCore.dll";
#else
			static_assert(false);
#endif
		}

		std::filesystem::path GetScriptModuleOuputPath()
		{
#if SK_DEBUG
			std::filesystem::path binaryOutputDir = Project::Directory() / "bin/Debug-windows-x86_64";
#elif SK_RELEASE
			std::filesystem::path binaryOutputDir = Project::Directory() / "bin/Release-windows-x86_64";
#else
			static_assert(false);
#endif
			std::wstring projectName = String::ToWideCopy(Project::GetActive()->GetConfig().Name);
			return fmt::format(L"{0}/{1}/{1}.dll", binaryOutputDir.native(), projectName);
		}

		MonoClass* GetClassFromName(const std::string className, MonoImage* image)
		{
			SK_PROFILE_FUNCTION();

			size_t seperator = className.find_last_of('.');

			std::string nameSpace;
			std::string name;
			if (seperator == std::string::npos)
			{
				nameSpace = "";
				name = className;
			}
			else
			{
				nameSpace = className.substr(0, seperator);
				name = className.substr(seperator + 1);
			}

			return mono_class_from_name(image, nameSpace.c_str(), name.c_str());
		}

		MonoClass* GetClassFromNameScript(const std::string className)
		{
			return GetClassFromName(className, s_ScriptData.ScriptImage);
		}
		
		MonoClass* GetClassFromNameCore(const std::string className)
		{
			return GetClassFromName(className, s_ScriptData.CoreImage);
		}
		
		MonoMethod* GetMethodFromClass(const std::string& methodName, bool includeNameSpace, MonoClass* clazz)
		{
			SK_PROFILE_FUNCTION();

			if (MonoMethodDesc* methodDesc = mono_method_desc_new(methodName.c_str(), includeNameSpace))
			{
				MonoMethod* method = mono_method_desc_search_in_class(methodDesc, clazz);
				mono_method_desc_free(methodDesc);
				return method;
			}
			return nullptr;
		}

		MonoMethod* GetVirtualMethod(const std::string& methodName, bool includeNameSpace, MonoClass* clazz, MonoObject* object)
		{
			SK_PROFILE_FUNCTION();

			if (MonoMethodDesc* methodDesc = mono_method_desc_new(methodName.c_str(), includeNameSpace))
			{
				MonoMethod* virtualMethod = mono_method_desc_search_in_class(methodDesc, clazz);
				MonoMethod* method = mono_object_get_virtual_method(object, virtualMethod);
				mono_method_desc_free(methodDesc);
				return method;
			}
			return nullptr;
		}

		void InvokeClassMethod(MonoObject* object, MonoMethod* method, void** args = nullptr)
		{
			SK_PROFILE_FUNCTION();

			MonoObject* exeption = nullptr;
			mono_runtime_invoke(method, object, args, &exeption);
			if (exeption)
			{
				MonoString* msg = mono_object_to_string(exeption, nullptr);
				SK_CORE_ERROR(mono_string_to_utf8(msg));
			}
		}

		spdlog::level::level_enum MonoLogLevelTospdlog(std::string_view logLevel)
		{
			if (logLevel == "debug")
				return spdlog::level::debug;
			if (logLevel == "info")
				return spdlog::level::trace;
			if (logLevel == "message")
				return spdlog::level::info;
			if (logLevel == "warning")
				return spdlog::level::warn;
			if (logLevel == "critical")
				return spdlog::level::critical;
			if (logLevel == "error")
				return spdlog::level::err;
			return spdlog::level::off;
		}

	}

	void LogCallback(const char* log_domain, const char* log_level, const char* message, mono_bool fatal, void* user_data)
	{
		auto level = utils::MonoLogLevelTospdlog(log_level);
		SK_CORE_LOG(level, "{}: {}", log_domain, message);
	}

	bool ScriptEngine::Init(const std::string& scriptinCoreAssemblyPath)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO("Init ScriptEngine [{}]", scriptinCoreAssemblyPath);

		mono_trace_set_level_string(SK_MONO_LOG_LEVEL);
		mono_trace_set_log_handler(&LogCallback, nullptr);

		s_ScriptData.CoreAssemblyPath = scriptinCoreAssemblyPath;
		mono_set_dirs(SK_CONNECT_TO_STRING(MONO_DIRECTORY, /lib), SK_CONNECT_TO_STRING(MONO_DIRECTORY, /etc));

		s_ScriptData.RootDomain = mono_jit_init("Core Domain");
		SK_CORE_ASSERT(s_ScriptData.RootDomain, "Failed to initialize Domain");

		return true;
	}

	void ScriptEngine::Shutdown()
	{
		SK_PROFILE_FUNCTION();

		if (s_ScriptData.RootDomain)
		{
			SK_CORE_INFO("ScriptEngine Shutdown");

			UnloadAssembly();

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
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO("ScriptEngine Assembly Loaded [{}]", assemblyPath);

		CheckForModuleUpdate();

		// Create Runtime Domain
		s_ScriptData.RuntimeDomain = mono_domain_create_appdomain("Script Domain", nullptr);
		SK_CORE_ASSERT(s_ScriptData.RuntimeDomain);
		mono_domain_set(s_ScriptData.RuntimeDomain, 0);

		// Core
		s_ScriptData.CoreAssembly = mono_domain_assembly_open(s_ScriptData.RuntimeDomain, s_ScriptData.CoreAssemblyPath.c_str());
		SK_CORE_ASSERT(s_ScriptData.CoreAssembly);

		s_ScriptData.CoreImage = mono_assembly_get_image(s_ScriptData.CoreAssembly);
		SK_CORE_ASSERT(s_ScriptData.CoreImage);

		// Scripting
		s_ScriptData.ScriptAssemblyPath = assemblyPath;
		s_ScriptData.ScriptAssembly = mono_domain_assembly_open(s_ScriptData.RuntimeDomain, assemblyPath.c_str());
		SK_CORE_ASSERT(s_ScriptData.ScriptAssembly);

		s_ScriptData.ScriptImage = mono_assembly_get_image(s_ScriptData.ScriptAssembly);
		SK_CORE_ASSERT(s_ScriptData.ScriptImage);

		s_ScriptData.AssemblyLoaded = true;

		MonoGlue::Glue();

		return true;
	}

	void ScriptEngine::UnloadAssembly()
	{
		SK_PROFILE_FUNCTION();

		if (!s_ScriptData.AssemblyLoaded)
			return;

		SK_CORE_ASSERT(s_ScriptData.RootDomain != s_ScriptData.RuntimeDomain);

		SK_CORE_INFO("ScriptEngine Assembly Unloaded");

		MonoGlue::UnGlue();

		s_ScriptData.AssemblyLoaded = false;

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
		SK_PROFILE_FUNCTION();

		UnloadAssembly();
		LoadAssembly(s_ScriptData.ScriptAssemblyPath);

		return true;
	}

	bool ScriptEngine::AssemblyLoaded()
	{
		return s_ScriptData.AssemblyLoaded;
	}

	void ScriptEngine::SetActiveScene(const Ref<Scene> scene)
	{
		s_ActiveScene = scene;
	}

	Ref<Scene> ScriptEngine::GetActiveScene()
	{
		return s_ActiveScene;
	}

	MonoImage* ScriptEngine::GetImage()
	{
		return s_ScriptData.ScriptImage;
	}

	MonoImage* ScriptEngine::GetCoreImage()
	{
		return s_ScriptData.CoreImage;
	}

	MonoClass* ScriptEngine::GetEntityClass()
	{
		return mono_class_from_name_case(s_ScriptData.CoreImage, "Shark", "Entity");
	}

	bool ScriptEngine::AssemblyHasScript(const std::string& className)
	{
		SK_PROFILE_FUNCTION();

		return utils::GetClassFromNameScript(className) != nullptr;
	}

	MonoMethod* ScriptEngine::GetMethod(const std::string& methodName, bool includeNameSpace)
	{
		SK_PROFILE_FUNCTION();

		return GetMethodInternal(methodName, includeNameSpace, s_ScriptData.ScriptImage);
	}

	MonoMethod* ScriptEngine::GetMethodCore(const std::string& methodName, bool includeNameSpace)
	{
		SK_PROFILE_FUNCTION();

		return GetMethodInternal(methodName, includeNameSpace, s_ScriptData.CoreImage);
	}

	MonoMethod* ScriptEngine::GetClassMethod(MonoClass* clazz, const std::string& methodName, bool includeNameSpace)
	{
		SK_PROFILE_FUNCTION();

		if (MonoMethodDesc* methodDesc = mono_method_desc_new(methodName.c_str(), includeNameSpace))
		{
			MonoMethod* method = mono_method_desc_search_in_class(methodDesc, clazz);
			mono_method_desc_free(methodDesc);
			return method;
		}
		return nullptr;
	}

	MonoObject* ScriptEngine::CallMethodInternal(MonoMethod* method, void* object, void** args)
	{
		SK_PROFILE_FUNCTION();

		MonoObject* exeption = nullptr;
		MonoObject* retVal = mono_runtime_invoke(method, object, args, &exeption);
		if (exeption)
		{
			MonoString* msg = mono_object_to_string(exeption, nullptr);
			SK_CORE_ERROR(mono_string_to_utf8(msg));
			return nullptr;
		}
		return retVal;
	}

	MonoMethod* ScriptEngine::GetMethodInternal(const std::string& methodName, bool includeNameSpace, MonoImage* image)
	{
		SK_PROFILE_FUNCTION();

		if (MonoMethodDesc* methodDesc = mono_method_desc_new(methodName.c_str(), includeNameSpace))
		{
			MonoMethod* method = mono_method_desc_search_in_image(methodDesc, image);
			mono_method_desc_free(methodDesc);
			return method;
		}
		return nullptr;
	}

	void ScriptEngine::CheckForModuleUpdate()
	{
		SK_PROFILE_FUNCTION();

		const std::array<std::pair<std::filesystem::path, std::filesystem::path>, 2> paths = {
			std::pair{ utils::GetScriptingCoreOuputPath(), Application::Get().GetSpecs().ScriptConfig.CoreAssemblyPath },
			std::pair{ utils::GetScriptModuleOuputPath(), Project::GetActive()->GetConfig().ScriptModulePath }
		};

		for (const auto& [source, target] : paths)
		{
			if (std::filesystem::exists(source) && std::filesystem::exists(target))
			{
				std::error_code err;

				std::filesystem::file_time_type sourceTime = std::filesystem::last_write_time(source, err);
				SK_CORE_ASSERT(!err, err.message());
				std::filesystem::file_time_type targetTime = std::filesystem::last_write_time(target, err);
				SK_CORE_ASSERT(!err, err.message());

				if (targetTime < sourceTime)
				{
					SK_CORE_WARN(L"Updating Module [{}]", source.filename().replace_extension().native());
					std::filesystem::copy_file(source, target, std::filesystem::copy_options::overwrite_existing, err);
					SK_CORE_ASSERT(!err, err.message());
				}
			}
		}
	}

}
