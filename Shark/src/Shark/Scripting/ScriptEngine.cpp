#include "skpch.h"
#include "ScriptEngine.h"

#include "Shark/Core/TimeStep.h"
#include "Shark/Core/Application.h"
#include "Shark/Core/Project.h"

#include "Shark/Scene/Components/ScriptComponent.h"

#include "Shark/Scripting/MonoGlue.h"

#include "Shark/Utility/String.h"

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

	struct ScriptClassData
	{
		uint32_t GCHandle = 0;
		MonoMethod* OnCreate = nullptr;
		MonoMethod* OnDestroy = nullptr;
		MonoMethod* OnUpdate = nullptr;
	};
	static std::unordered_map<UUID, ScriptClassData> s_ScriptClasses;
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
			std::filesystem::path binaryOutputDir = Project::GetProjectDirectory() / "bin/Debug-windows-x86_64";
#elif SK_RELEASE
			std::filesystem::path binaryOutputDir = Project::GetProjectDirectory() / "bin/Release-windows-x86_64";
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

		MonoGlue::Glue();

		// Scripting
		s_ScriptData.ScriptAssemblyPath = assemblyPath;
		s_ScriptData.ScriptAssembly = mono_domain_assembly_open(s_ScriptData.RuntimeDomain, assemblyPath.c_str());
		SK_CORE_ASSERT(s_ScriptData.ScriptAssembly);

		s_ScriptData.ScriptImage = mono_assembly_get_image(s_ScriptData.ScriptAssembly);
		SK_CORE_ASSERT(s_ScriptData.ScriptImage);

		s_ScriptData.AssemblyLoaded = true;

		return true;
	}

	void ScriptEngine::UnloadAssembly()
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(s_ScriptData.RootDomain != s_ScriptData.RuntimeDomain);

		SK_CORE_INFO("ScriptEngine Assembly Unloaded");

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

	bool ScriptEngine::HasScriptClass(const std::string& className)
	{
		SK_PROFILE_FUNCTION();

		return (bool)utils::GetClassFromNameScript(className);
	}

	bool ScriptEngine::InstantiateEntity(Entity entity)
	{
		if (!entity.HasComponent<ScriptComponent>())
			return false;

		auto& comp = entity.GetComponent<ScriptComponent>();
		SK_CORE_ASSERT(comp.ScriptModuleFound);
		if (CreateScript(comp.ScriptName, entity.GetUUID()))
		{
			comp.Handle = entity.GetUUID();
			return true;
		}
		return false;
	}

	bool ScriptEngine::CreateScript(const std::string& scriptName, UUID entityUUID)
	{
		MonoClass* clazz = utils::GetClassFromNameScript(scriptName);
		SK_CORE_ASSERT(clazz);

		return CreateScript(clazz, entityUUID);
	}

	bool ScriptEngine::CreateScript(MonoClass* scriptClass, UUID entityUUID)
	{
		SK_PROFILE_FUNCTION();

		MonoClass* entityClass = utils::GetClassFromNameCore("Shark.Entity");
		SK_CORE_ASSERT(entityClass);

		const char* scriptName = mono_class_get_name(scriptClass);

		if (!mono_class_is_subclass_of(scriptClass, entityClass, false))
		{
			SK_CORE_ERROR("{} must derive from Shark.Entity", scriptName);
			return false;
		}

		SK_CORE_ASSERT(entityUUID.IsValid());
		ScriptClassData& data = s_ScriptClasses[entityUUID];
		SK_CORE_ASSERT(!data.GCHandle, "Script Allready Created");

		// Create Script Object
		MonoObject* object = mono_object_new(mono_domain_get(), scriptClass);
		data.GCHandle = mono_gchandle_new(object, false);

		// Init Object
		mono_runtime_object_init(object);
		MonoClassField* handleField = mono_class_get_field_from_name(entityClass, "m_Handle");
		mono_field_set_value(object, handleField, &entityUUID);

		// Get Functions
		data.OnCreate = utils::GetMethodFromClass(fmt::format("{}:OnCreate()", scriptName), true, scriptClass);
		data.OnDestroy = utils::GetMethodFromClass(fmt::format("{}:OnDestroy()", scriptName), true, scriptClass);
		data.OnUpdate = utils::GetMethodFromClass(fmt::format("{}:OnUpdate(Shark.TimeStep)", scriptName), true, scriptClass);

		if (data.OnCreate)
			utils::InvokeClassMethod(object, data.OnCreate);
		return true;
	}

	void ScriptEngine::DestroyScript(UUID handle)
	{
		SK_PROFILE_FUNCTION();

		ScriptClassData& data = s_ScriptClasses.at(handle);
		if (data.OnDestroy)
		{
			MonoObject* object = mono_gchandle_get_target(data.GCHandle);
			utils::InvokeClassMethod(object, data.OnDestroy);
		}
		mono_gchandle_free(data.GCHandle);
		s_ScriptClasses.erase(handle);
	}

	void ScriptEngine::UpdateScript(UUID handle, TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		const ScriptClassData& data = s_ScriptClasses.at(handle);
		if (data.OnUpdate)
		{
			void* args[] = {
				&ts
			};

			MonoObject* object = mono_gchandle_get_target(data.GCHandle);
			utils::InvokeClassMethod(object, data.OnUpdate, args);
		}
	}

	bool ScriptEngine::IsValidScriptUUID(UUID handle)
	{
		return s_ScriptClasses.find(handle) != s_ScriptClasses.end();
	}

	MonoObject* ScriptEngine::GetScriptObject(UUID handle)
	{
		auto& data = s_ScriptClasses.at(handle);
		return mono_gchandle_get_target(data.GCHandle);
	}

	MonoMethod* ScriptEngine::GetMethod(const std::string& methodName, bool includeNameSpace)
	{
		return GetMethodInternal(methodName, includeNameSpace, s_ScriptData.ScriptImage);
	}

	MonoMethod* ScriptEngine::GetMethodCore(const std::string& methodName, bool includeNameSpace)
	{
		return GetMethodInternal(methodName, includeNameSpace, s_ScriptData.CoreImage);
	}

	MonoObject* ScriptEngine::CallMethod(MonoMethod* method, void* object, void** args)
	{
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

	MonoMethod* ScriptEngine::GetMethodInternal(const std::string methodName, bool includeNameSpace, MonoImage* image)
	{
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

