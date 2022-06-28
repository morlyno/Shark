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

#define SK_MONO_LOG_LEVEL "warning"

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

	static Ref<Scene> s_ActiveScene = nullptr;
	static BuildConfiguration s_BuildConfig = BuildConfiguration::None;
	static bool s_ForceModuleUpdate = false;

	namespace utils {

		std::filesystem::path GetScriptingCoreOuputPath()
		{
			switch (s_BuildConfig)
			{
				case BuildConfiguration::Debug:   return (std::filesystem::current_path() / "../bin/Debug-windows-x86_64/ScriptingCore/ScriptingCore.dll").lexically_normal();
				case BuildConfiguration::Release: return (std::filesystem::current_path() / "../bin/Release-windows-x86_64/ScriptingCore/ScriptingCore.dll").lexically_normal();
			}

			SK_CORE_ASSERT(false, "Invalid Build Configuration");
			return std::filesystem::path{};
		}

		std::filesystem::path GetScriptModuleOuputPath()
		{
			std::filesystem::path path;
			switch (s_BuildConfig)
			{
				case BuildConfiguration::Debug:   path = Project::Directory() / "bin/Debug-windows-x86_64"; break;
				case BuildConfiguration::Release: path = Project::Directory() / "bin/Release-windows-x86_64"; break;
				default:
					SK_CORE_ASSERT(false, "Invalid Build Configuration");
					return std::filesystem::path{};
			}

			std::wstring projectName = String::ToWideCopy(Project::GetActive()->GetConfig().Name);
			return fmt::format(L"{0}/{1}/{1}.dll", path, projectName);
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

		Log::Level MonoLogLevelToLogLevel(std::string_view logLevel)
		{
			if (logLevel == "debug")
				return Log::Level::Debug;
			if (logLevel == "info")
				return Log::Level::Trace;
			if (logLevel == "message")
				return Log::Level::Info;
			if (logLevel == "warning")
				return Log::Level::Warn;
			if (logLevel == "critical")
				return Log::Level::Critical;
			if (logLevel == "error")
				return Log::Level::Error;
			return Log::Level::Trace;
		}

	}

	std::string ToString(BuildConfiguration buildConfig)
	{
		switch (buildConfig)
		{
			case BuildConfiguration::None:    return "None";
			case BuildConfiguration::Debug:   return "Debug";
			case BuildConfiguration::Release: return "Release";
		}

		SK_CORE_ASSERT(false, "Unkown Build Configuration");
		return "Unkown";
	}

	void ScriptEngine::HandleException(MonoObject* exception)
	{
		if (!exception)
			return;

		MonoString* monoStr = mono_object_to_string(exception, nullptr);
		const wchar_t* str = mono_string_to_utf16(monoStr);
		SK_CONSOLE_ERROR(str);
	}

	void LogCallback(const char* log_domain, const char* log_level, const char* message, mono_bool fatal, void* user_data)
	{
		auto level = utils::MonoLogLevelToLogLevel(log_level);
		SK_CORE_LOG(level, "{}: {}{}", log_domain ? log_domain : "Unkown Domain", fatal ? "[Fatal] " : "", message);
	}

	bool ScriptEngine::Init(const std::string& scriptingCoreAssemblyPath)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO("Init ScriptEngine [{}]", scriptingCoreAssemblyPath);

		mono_trace_set_level_string(SK_MONO_LOG_LEVEL);
		mono_trace_set_log_handler(&LogCallback, nullptr);

		s_ScriptData.CoreAssemblyPath = scriptingCoreAssemblyPath;
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

			mono_jit_cleanup(s_ScriptData.RootDomain);
			s_ScriptData.RootDomain = nullptr;
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

		s_ScriptData.ScriptAssemblyPath = assemblyPath;

		if (s_ForceModuleUpdate)
		{
			UpdateModules();
			s_ForceModuleUpdate = false;
		}
		else
		{
			CheckForModuleUpdate();
		}

		s_ScriptData.RuntimeDomain = mono_domain_create_appdomain("Script Domain", nullptr);
		if (!s_ScriptData.RuntimeDomain)
		{
			SK_CORE_ERROR("Failed to load Runtime Domain");
			return false;
		}
		mono_domain_set(s_ScriptData.RuntimeDomain, 0);
		
		s_ScriptData.CoreAssembly = mono_domain_assembly_open(s_ScriptData.RuntimeDomain, s_ScriptData.CoreAssemblyPath.c_str());
		if (!s_ScriptData.CoreAssembly)
		{
			SK_CORE_ERROR("Failed to open Core Assembly");
			return false;
		}

		s_ScriptData.CoreImage = mono_assembly_get_image(s_ScriptData.CoreAssembly);
		if (!s_ScriptData.CoreImage)
		{
			SK_CORE_ERROR("Failed to get Core Image");
			return false;
		}


		s_ScriptData.ScriptAssembly = mono_domain_assembly_open(s_ScriptData.RuntimeDomain, s_ScriptData.ScriptAssemblyPath.c_str());
		if (!s_ScriptData.ScriptAssembly)
		{
			SK_CORE_ERROR("Failed to open Script Assembly");
			return false;
		}

		s_ScriptData.ScriptImage = mono_assembly_get_image(s_ScriptData.ScriptAssembly);
		if (!s_ScriptData.ScriptImage)
		{
			SK_CORE_ERROR("Failed to get Script image");
			return false;
		}

		MonoGlue::Init();

		SK_CORE_INFO("ScriptEngine Assembly Loaded [{}]", s_ScriptData.ScriptAssemblyPath);
		return true;
	}

	void ScriptEngine::UnloadAssembly()
	{
		SK_PROFILE_FUNCTION();

		if (!s_ScriptData.RuntimeDomain)
			return;

		SK_CORE_INFO("ScriptEngine Assembly Unloaded [{}]", s_ScriptData.ScriptAssemblyPath);

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
		return LoadAssembly(s_ScriptData.ScriptAssemblyPath);
	}

	bool ScriptEngine::ReloadIfNeeded()
	{
		if (AnyModuleNeedsUpdate())
		{
			UnloadAssembly();
			return LoadAssembly(s_ScriptData.ScriptAssemblyPath);
		}
		return false;
	}

	bool ScriptEngine::AssemblyLoaded()
	{
		return s_ScriptData.ScriptAssembly;
	}

	void ScriptEngine::SetBuildConfiguration(BuildConfiguration buildConfig)
	{
		s_BuildConfig = buildConfig;
		s_ForceModuleUpdate = true;
	}

	BuildConfiguration ScriptEngine::GetBuildConfiguration()
	{
		return s_BuildConfig;
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
		if (!s_ScriptData.ScriptAssembly)
			return false;

		return utils::GetClassFromName(className, s_ScriptData.ScriptImage) != nullptr;
	}

	MonoMethod* ScriptEngine::GetMethod(const std::string& methodName, bool includeNameSpace)
	{
		return GetMethodInternal(methodName, includeNameSpace, s_ScriptData.ScriptImage);
	}

	MonoMethod* ScriptEngine::GetMethodCore(const std::string& methodName, bool includeNameSpace)
	{
		return GetMethodInternal(methodName, includeNameSpace, s_ScriptData.CoreImage);
	}

	MonoMethod* ScriptEngine::GetClassMethod(MonoClass* clazz, const std::string& methodName, bool includeNameSpace)
	{
		if (MonoMethodDesc* methodDesc = mono_method_desc_new(methodName.c_str(), includeNameSpace))
		{
			MonoMethod* method = mono_method_desc_search_in_class(methodDesc, clazz);
			mono_method_desc_free(methodDesc);
			return method;
		}
		return nullptr;
	}

	MonoObject* ScriptEngine::InvokeMethodInternal(MonoMethod* method, void* object, void** args)
	{
		MonoObject* exception = nullptr;
		MonoObject* retVal = mono_runtime_invoke(method, object, args, &exception);
		if (exception)
		{
			HandleException(exception);
			return nullptr;
		}
		return retVal;
	}

	MonoMethod* ScriptEngine::GetMethodInternal(const std::string& methodName, bool includeNameSpace, MonoImage* image)
	{
		if (MonoMethodDesc* methodDesc = mono_method_desc_new(methodName.c_str(), includeNameSpace))
		{
			MonoMethod* method = mono_method_desc_search_in_image(methodDesc, image);
			mono_method_desc_free(methodDesc);
			return method;
		}
		return nullptr;
	}

	void ScriptEngine::UpdateModules()
	{
		SK_PROFILE_FUNCTION();

		const std::array<std::pair<std::filesystem::path, std::filesystem::path>, 2> paths = {
			std::pair{ utils::GetScriptingCoreOuputPath(), Application::Get().GetSpecs().ScriptConfig.CoreAssemblyPath },
			std::pair{ utils::GetScriptModuleOuputPath(), Project::GetActive()->GetConfig().ScriptModulePath }
		};

		for (const auto& [source, target] : paths)
		{
			if (std::filesystem::exists(source))
			{
				std::error_code err;

				auto parentPath = target.parent_path();
				if (!std::filesystem::exists(parentPath))
					std::filesystem::create_directories(parentPath);

				SK_CORE_WARN(L"Updating Module [{}]", source.filename().replace_extension().native());
				std::filesystem::copy_file(source, target, std::filesystem::copy_options::update_existing, err);
				SK_CORE_ASSERT(!err, err.message());
			}
		}
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
			if (std::filesystem::exists(source))
			{
				std::error_code err;

				if (std::filesystem::exists(target))
				{
					std::filesystem::file_time_type sourceTime = std::filesystem::last_write_time(source, err);
					SK_CORE_ASSERT(!err, err.message());
					std::filesystem::file_time_type targetTime = std::filesystem::last_write_time(target, err);
					SK_CORE_ASSERT(!err, err.message());

					if (targetTime < sourceTime)
					{
						SK_CORE_WARN(L"Updating Module [{}]", source.filename().replace_extension().native());
						std::filesystem::copy_file(source, target, std::filesystem::copy_options::update_existing, err);
						SK_CORE_ASSERT(!err, err.message());
					}
				}
				else
				{
					auto parentPath = target.parent_path();
					if (!std::filesystem::exists(parentPath))
						std::filesystem::create_directories(parentPath);

					SK_CORE_WARN(L"Copying Module [{}]", source.filename().replace_extension().native());
					std::filesystem::copy_file(source, target, err);
					SK_CORE_ASSERT(!err, err.message());
				}

			}
		}
	}

	bool ScriptEngine::AnyModuleNeedsUpdate()
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
					return true;
			}
		}
		return false;
	}

}

